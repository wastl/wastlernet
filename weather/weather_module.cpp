//
// Created by wastl on 08.04.23.
//

#include "weather_module.h"
#include "weather_listener.h"

namespace weather {
    void WeatherModule::Start() {
        listener = start_listener(uri, [this](const WeatherData& data) {
            auto st = Update(data);
            if (!st.ok()) {
                LOG(ERROR) << "Error: " << st;
            }
        });
    }

    void WeatherModule::Abort() {
        listener->close();
    }

    void WeatherModule::Wait() {
        using namespace std::chrono_literals;
        while(true) {
            std::this_thread::sleep_for(1s);
        }

    }
}