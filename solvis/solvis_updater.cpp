//
// Created by wastl on 27.10.23.
//

#include "solvis_updater.h"
#include "base/utility.h"

#include "weather/weather.pb.h"
#include <glog/logging.h>
#include <absl/strings/str_cat.h>
#include <modbus/modbus.h>
#include <modbus/modbus-tcp.h>

#define LOGS(level) LOG(level) << "[solvis] "

absl::Status solvis::SolvisUpdater::Update() {
    if (auto it = current_state_->find("weather"); it != current_state_->cend()) {
        weather::WeatherData weather;
        weather.ParseFromString(it->second);
        if (weather.has_indoor()) {
            // SOLVIS modbus registers store temperature in units of 0.1
            uint16_t indoor_temperature = weather.indoor().temperature() * 10;

            return conn_->Execute([indoor_temperature](modbus_t* ctx) {

                LOGS(INFO) << "Updating SOLVIS indoor temperature to value " << indoor_temperature / 10.0;
                int rc1 = modbus_write_register(ctx, 34304, indoor_temperature);
                int rc2 = modbus_write_register(ctx, 34305, indoor_temperature);
                int rc3 = modbus_write_register(ctx, 34306, indoor_temperature);

                if (rc1 == -1 || rc2 == -1 || rc3 == -1) {
                    LOGS(ERROR) << "Error writing register: " << modbus_strerror(errno);
                    return absl::InternalError(absl::StrCat("Error writing register: ", modbus_strerror(errno)));
                } else {
                    return absl::OkStatus();
                }
            });

        } else {
            return absl::NotFoundError("Indoor temperature information not found, not updating SOLVIS");
        }
    }

    return absl::NotFoundError("Weather information not found, not updating SOLVIS");
}
