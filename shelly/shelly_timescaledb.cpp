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
    temperature,
    humidity
) VALUES ($1,$2))");

    conn.prepare("shelly_insert_temperature_temp_only", R"(
INSERT INTO shelly_temperature(
    temperature,
    humidity
) VALUES ($1,NULL))");

    conn.prepare("shelly_insert_temperature_hum_only", R"(
INSERT INTO shelly_temperature(
    temperature,
    humidity
) VALUES (NULL,$1))");

    // Light combinations
    conn.prepare("shelly_insert_light_both", R"(
INSERT INTO shelly_light(
    lux,
    illumination
) VALUES ($1,$2))");

    conn.prepare("shelly_insert_light_lux_only", R"(
INSERT INTO shelly_light(
    lux,
    illumination
) VALUES ($1,NULL))");

    conn.prepare("shelly_insert_light_illum_only", R"(
INSERT INTO shelly_light(
    lux,
    illumination
) VALUES (NULL,$1))");

    return absl::OkStatus();
}

absl::Status ShellyWriter::write(pqxx::work &tx, const ShellyData &data) {
    // Insert temperature/humidity if provided
    if (data.has_temperature_data()) {
        const auto &t = data.temperature_data();
        const bool ht = t.has_temperature();
        const bool hh = t.has_humidity();
        if (ht && hh) {
            tx.exec(pqxx::prepped{"shelly_insert_temperature_both"}, pqxx::params{ t.temperature(), t.humidity() });
        } else if (ht && !hh) {
            tx.exec(pqxx::prepped{"shelly_insert_temperature_temp_only"}, pqxx::params{ t.temperature() });
        } else if (!ht && hh) {
            tx.exec(pqxx::prepped{"shelly_insert_temperature_hum_only"}, pqxx::params{ t.humidity() });
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
            tx.exec(pqxx::prepped{"shelly_insert_light_both"}, pqxx::params{ l.lux(), l.illumination() });
        } else if (hlx && !hil) {
            tx.exec(pqxx::prepped{"shelly_insert_light_lux_only"}, pqxx::params{ l.lux() });
        } else if (!hlx && hil) {
            tx.exec(pqxx::prepped{"shelly_insert_light_illum_only"}, pqxx::params{ l.illumination() });
        } else {
            // neither provided: skip
        }
    }

    return absl::OkStatus();
}

} // namespace wastlernet::shelly
