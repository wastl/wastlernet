//
// Created by wastl on 26.11.25.
//

#include "shelly_timescaledb.h"

namespace wastlernet::shelly {

absl::Status ShellyWriter::prepare(pqxx::connection &conn) {
    // Prepared statements; time defaults to CURRENT_TIMESTAMP in schema
    // Temperature combinations
    conn.prepare("shelly_insert_temperature_both", R"(
INSERT INTO shelly_temperature(
    device,
    temperature,
    humidity
) VALUES ($1,$2,$3))");

    conn.prepare("shelly_insert_temperature_temp_only", R"(
INSERT INTO shelly_temperature(
    device,
    temperature,
    humidity
) VALUES ($1,$2,NULL))");

    conn.prepare("shelly_insert_temperature_hum_only", R"(
INSERT INTO shelly_temperature(
    device,
    temperature,
    humidity
) VALUES ($1,NULL,$2))");

    // Light combinations
    conn.prepare("shelly_insert_light_both", R"(
INSERT INTO shelly_light(
    device,
    lux,
    illumination
) VALUES ($1,$2,$3))");

    conn.prepare("shelly_insert_light_lux_only", R"(
INSERT INTO shelly_light(
    device,
    lux,
    illumination
) VALUES ($1,$2,NULL))");

    conn.prepare("shelly_insert_light_illum_only", R"(
INSERT INTO shelly_light(
    device,
    lux,
    illumination
) VALUES ($1,NULL,$2))");

    // Energy: all fields present are required by our module before writing
    conn.prepare("shelly_insert_energy_all", R"(
INSERT INTO shelly_energy(
    device,
    power,
    voltage,
    current,
    frequency
) VALUES ($1,$2,$3,$4,$5))");

    // Motion: single boolean value plus device
    conn.prepare("shelly_insert_motion", R"(
INSERT INTO shelly_motion(
    device,
    motion
) VALUES ($1,$2))");

    return absl::OkStatus();
}

absl::Status ShellyWriter::write(pqxx::work &tx, const ShellyData &data) {
    // Device name must be stored in the NOT NULL device column
    std::string device = data.has_device_name() ? data.device_name() : std::string();
    if (device.empty()) {
        // If device is unknown, skip inserts to satisfy NOT NULL constraint
        // This situation should not normally occur because the module sets device_name from topic/src
        return absl::InvalidArgumentError("ShellyData.device_name is empty; cannot write to TimescaleDB");
    }
    // Insert temperature/humidity if provided
    if (data.has_temperature_data()) {
        const auto &t = data.temperature_data();
        const bool ht = t.has_temperature();
        const bool hh = t.has_humidity();
        if (ht && hh) {
            tx.exec(pqxx::prepped{"shelly_insert_temperature_both"}, pqxx::params{ device, t.temperature(), t.humidity() });
        } else if (ht && !hh) {
            tx.exec(pqxx::prepped{"shelly_insert_temperature_temp_only"}, pqxx::params{ device, t.temperature() });
        } else if (!ht && hh) {
            tx.exec(pqxx::prepped{"shelly_insert_temperature_hum_only"}, pqxx::params{ device, t.humidity() });
        } else {
            // neither provided: skip
        }
    }

    // Insert light if provided
    if (data.has_light_data()) {
        const auto &l = data.light_data();
        const bool hlx = l.has_lux();
        const bool hil = l.has_illumination();
        if (hlx && hil) {
            tx.exec(pqxx::prepped{"shelly_insert_light_both"}, pqxx::params{ device, l.lux(), l.illumination() });
        } else if (hlx && !hil) {
            tx.exec(pqxx::prepped{"shelly_insert_light_lux_only"}, pqxx::params{ device, l.lux() });
        } else if (!hlx && hil) {
            tx.exec(pqxx::prepped{"shelly_insert_light_illum_only"}, pqxx::params{ device, l.illumination() });
        } else {
            // neither provided: skip
        }
    }

    // Insert energy if provided (we only write when all fields are set by the module)
    if (data.has_energy_data()) {
        const auto &e = data.energy_data();
        // The module currently populates all four fields together; insert them as-is.
        tx.exec(
            pqxx::prepped{"shelly_insert_energy_all"},
            pqxx::params{ device, e.power(), e.voltage(), e.current(), e.frequency() }
        );
    }

    // Insert motion if provided
    if (data.has_motion_data()) {
        const auto &m = data.motion_data();
        if (m.has_motion()) {
            tx.exec(
                pqxx::prepped{"shelly_insert_motion"},
                pqxx::params{ device, m.motion() }
            );
        }
    }

    return absl::OkStatus();
}

} // namespace wastlernet::shelly
