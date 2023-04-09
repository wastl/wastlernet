//
// Created by wastl on 19.01.22.
//
#include <functional>
#include "weather/weather.pb.h"

#ifndef WEATHER_EXPORTER_WEATHER_LISTENER_H
#define WEATHER_EXPORTER_WEATHER_LISTENER_H
using web::http::experimental::listener::http_listener;

namespace weather {
    std::unique_ptr<http_listener> start_listener(const std::string &uri, const std::function<void(const WeatherData &)> &handler);
}
#endif //WEATHER_EXPORTER_WEATHER_LISTENER_H
