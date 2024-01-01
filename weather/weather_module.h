//
// Created by wastl on 08.04.23.
//
#include <cpprest/http_listener.h>

#include "include/module.h"
#include "weather/weather.pb.h"
#include "weather/weather_timescaledb.h"

#ifndef WASTLERNET_WEATHER_MODULE_H
#define WASTLERNET_WEATHER_MODULE_H
namespace weather {
    class WeatherModule : public wastlernet::Module<WeatherData> {
    private:
        std::string uri;

        std::unique_ptr<web::http::experimental::listener::http_listener> listener;
    protected:
    public:
        void Start() override;

        void Abort() override;

        void Wait() override;

    public:
        WeatherModule(const wastlernet::TimescaleDB &db_cfg, const wastlernet::Weather& client_cfg, wastlernet::StateCache* c)
                : wastlernet::Module<WeatherData>(db_cfg, new WeatherWriter, c),
                  uri(client_cfg.listen()) { }

        std::string Name() override {
            return "weather";
        }
    };
}
#endif //WASTLERNET_WEATHER_MODULE_H
