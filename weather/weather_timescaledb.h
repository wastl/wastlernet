//
// Created by wastl on 08.04.23.
//
#include <pqxx/pqxx>
#include <absl/status/status.h>
#include "timescaledb/timescaledb-client.h"
#include "weather/weather.pb.h"

#ifndef WASTLERNET_WEATHER_TIMESCALEDB_H
#define WASTLERNET_WEATHER_TIMESCALEDB_H
namespace weather {
    class WeatherWriter : public timescaledb::TimescaleWriter<WeatherData> {
    public:
        absl::Status prepare(pqxx::connection &conn) override;

        absl::Status write(pqxx::work &tx, const WeatherData &data) override;
    };
}
#endif //WASTLERNET_WEATHER_TIMESCALEDB_H
