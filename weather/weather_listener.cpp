//
// Created by wastl on 19.01.22.
//
#include <chrono>
#include <thread>
#include <cpprest/http_msg.h>
#include <cpprest/http_listener.h>
#include <glog/logging.h>

#include "weather_listener.h"

namespace {
    double fahrenheit2celsius(double fahrenheit) {
        return 5 * (fahrenheit - 32) / 9;
    }

    double mph2ms(double mph) {
        double kmh = mph / 0.6213712;
        return kmh * 1000 / 3600;
    }

    double inch2mm(double inch) {
        return inch / 0.03937007874;
    }
}

using web::http::http_request;
using web::http::experimental::listener::http_listener;

namespace weather {
    std::unique_ptr<http_listener> start_listener(const std::string& uri, const std::function<void(const WeatherData&)>& handler) {
        auto listener = std::make_unique<http_listener>(uri);

        listener->support([=](http_request request){
            auto uri = request.relative_uri();
            auto q = web::uri::split_query(uri.query());

            LOG(INFO) << "Wetterdaten erhalten";

            try {
                WeatherData data;
                data.set_uv(std::stod(q["UV"]));
                data.set_barometer(inch2mm(std::stod(q["baromin"])) * 1.33322);
                data.set_dailyrain(inch2mm(std::stod(q["dailyrainin"])));
                data.set_dewpoint(fahrenheit2celsius(std::stod(q["dewptf"])));
                data.mutable_outdoor()->set_humidity(std::stod(q["humidity"]));
                data.mutable_outdoor()->set_temperature(fahrenheit2celsius(std::stod(q["tempf"])));
                data.mutable_indoor()->set_humidity(std::stod(q["indoorhumidity"]));
                data.mutable_indoor()->set_temperature(fahrenheit2celsius(std::stod(q["indoortempf"])));
                data.set_rain(inch2mm(std::stod(q["rainin"])));
                data.set_solarradiation(std::stod(q["solarradiation"]));
                data.mutable_wind()->set_direction(std::stoi(q["winddir"]));
                data.mutable_wind()->set_gusts(mph2ms(std::stod(q["windgustmph"])));
                data.mutable_wind()->set_speed(mph2ms(std::stod(q["windspeedmph"])));

                handler(data);

                request.reply(web::http::status_codes::OK).get();
            } catch(const std::exception &e) {
                LOG(ERROR) << "Error while logging data: " << e.what();
                request.reply(web::http::status_codes::InternalError);
            }
        });

        listener->open().get();

        return std::move(listener);
    }
}