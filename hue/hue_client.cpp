//
// Created by wastl on 07.07.24.
//

#include "hue_client.h"

#include <cmath>
#include <glog/logging.h>
#include <hueplusplus/Bridge.h>
#include <hueplusplus/LinHttpHandler.h>
#include <hueplusplus/Sensor.h>
#include <hueplusplus/ZLLSensors.h>
#include <absl/container/flat_hash_map.h>
#include <absl/strings/str_split.h>


absl::Status hue::query(const wastlernet::Hue& hueCfg, const std::function<absl::Status(const hue::HueData&)>& handler) {
    hue::HueData data;

    if (hueCfg.bridge_size() > 0) {
        auto handler = std::make_shared<hueplusplus::LinHttpHandler>();

        for (const auto& bridgecfg : hueCfg.bridge()) {
            hueplusplus::Bridge bridge(bridgecfg.host(), bridgecfg.port(), bridgecfg.username(), handler, bridgecfg.client_key());

            for (const auto& group : bridge.groups().getAll()) {
                if (group.getType() == "Room") {
                    auto roomData = data.add_rooms();
                    roomData->set_name(group.getName());
                    roomData->set_all_on(group.getAllOn());
                    roomData->set_any_on(group.getAnyOn());

                    for (int lightId: group.getLightIds()) {
                        hueplusplus::Light light = bridge.lights().get(lightId);

                        auto lightData = roomData->add_lights();
                        lightData->set_name(light.getName());
                        lightData->set_on(light.isOn());
                        lightData->set_uid(light.getUId());
                        lightData->set_brightness(light.getBrightness());
                    }
                }
            }

            absl::flat_hash_map<std::string, hue::Sensor> sensors;

            // Pass one: get sensor names
            for (const auto& sensor : bridge.sensors().getAll()) {
                if (sensor.getType().substr(0,3) == "ZLL") {
                    std::vector<std::string> id_parts = absl::StrSplit(sensor.getUId(), "-");

                    // ZLL main sensor has the name and ends with id 0406.
//                    if (id_parts.size() == 3 && id_parts[2] == "0406") {
                    if (id_parts.size() > 0 && sensor.isPrimary()) {
                        sensors[id_parts[0]].set_name(sensor.getName());
                        sensors[id_parts[0]].set_uid(sensor.getUId());
                        sensors[id_parts[0]].set_reachable(sensor.isReachable());
                    } else if(id_parts.size() != 3) {
                        LOG(ERROR) << "ZLL Sensor with wrong UID format: " << sensor.getUId() << " " << sensor.getName() << " " << sensor.getType();
                    }
                }
            }

            // Pass two: get sensor data

            // Presence
            for (const auto& sensor : bridge.sensors().getAllByType<hueplusplus::sensors::ZLLPresence>()) {
                if (sensor.getType().substr(0,3) == "ZLL") {
                    std::vector<std::string> id_parts = absl::StrSplit(sensor.getUId(), "-");
                    sensors[id_parts[0]].set_presence(sensor.getPresence());
                }
            }

            // Temperature
            for (const auto& sensor : bridge.sensors().getAllByType<hueplusplus::sensors::ZLLTemperature>()) {
                if (sensor.getType().substr(0,3) == "ZLL") {
                    std::vector<std::string> id_parts = absl::StrSplit(sensor.getUId(), "-");

                    sensors[id_parts[0]].set_temperature(sensor.getTemperature()/100.0);
                }
            }

            // Light Level
            for (const auto& sensor : bridge.sensors().getAllByType<hueplusplus::sensors::ZLLLightLevel>()) {
                if (sensor.getType().substr(0,3) == "ZLL") {
                    std::vector<std::string> id_parts = absl::StrSplit(sensor.getUId(), "-");

                    sensors[id_parts[0]].set_daylight(sensor.isDaylight());
                    sensors[id_parts[0]].set_dark(sensor.isDark());
                    sensors[id_parts[0]].set_lightlevel(pow(10, (sensor.getLightLevel()-1)/10000.0));
                }
            }

            // Pass three: add to proto
            for (auto& kv : sensors) {
                data.add_sensors()->Swap(&kv.second);
            }
        }
    }
    return handler(data);
}

