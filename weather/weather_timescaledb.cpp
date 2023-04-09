//
// Created by wastl on 08.04.23.
//

#include "weather_timescaledb.h"

namespace weather {
    absl::Status WeatherWriter::write(pqxx::work &tx, const WeatherData &data) {
        tx.exec_prepared("weather_insert",
                         data.uv(),
                         data.barometer(),
                         data.dailyrain(),
                         data.dewpoint(),
                         data.outdoor().temperature(),
                         data.outdoor().humidity(),
                         data.indoor().temperature(),
                         data.indoor().humidity(),
                         data.wind().direction(),
                         data.wind().speed(),
                         data.wind().gusts(),
                         data.rain(),
                         data.solarradiation()
        );
        return absl::OkStatus();
    }

    absl::Status WeatherWriter::prepare(pqxx::connection &conn) {
        conn.prepare("weather_insert", R"(
INSERT INTO weather (
    uv,
    barometer,
    daily_rain,
    dewpoint,
    outdoor_temperature,
    outdoor_humidity,
    indoor_temperature,
    indoor_humidity,
    wind_direction,
    wind_speed,
    wind_gusts,
    rain,
    solarradiation
) VALUES ($1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12,$13))"
        );
        return absl::OkStatus();
    }
}