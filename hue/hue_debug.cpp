//
// Created by wastl on 07.07.24.
//
#include <iostream>

#include <google/protobuf/text_format.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/util/json_util.h>
#include <fcntl.h>
#include <unistd.h>
#include <cmath>

#include "config/config.pb.h"
#include "hue/hue_client.h"

using google::protobuf::io::ZeroCopyInputStream;
using google::protobuf::io::FileInputStream;
using namespace google::protobuf::util;

int main(int argc, char *argv[]) {
    wastlernet::Config config;

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        std::cerr << "Could not open configuration file" << std::endl;
        return 1;
    }
    ZeroCopyInputStream* cfg_file = new FileInputStream(fd);
    if (!google::protobuf::TextFormat::Parse(cfg_file, &config)) {
        std::cerr << "Unable to parse configuration file" << std::endl;
        return 1;
    }
    delete cfg_file;
    close(fd);

    std::cout << "Loaded configuration." << std::endl;

    auto st = hue::query(config.hue(),[](const hue::HueData& data) {
        std::cout << "Zimmer:" << std::endl;
        for (const hue::Room& room : data.rooms()) {
            std::cout << "- " << room.name() << ": " << (room.any_on()?"on":"off") << std::endl;
            for (const hue::Light& light : room.lights()) {
                std::cout << "  - " << light.name() << ": " << (light.on()?"on":"off") << std::endl;
            }
        }

        std::cout << "Sensoren:" << std::endl;
        for (const hue::Sensor& sensor : data.sensors()) {
            std::cout << "  - " << sensor.name() << ": ";
            if (sensor.has_daylight() && sensor.daylight()) {
                std::cout << "Tageslicht, ";
            }
            if (sensor.has_dark() && sensor.dark()) {
                std::cout << "Dunkelheit, ";
            }
            if (sensor.has_lightlevel()) {
                std::cout << "Lichtstärke: " << sensor.lightlevel()  << " lux, ";
            }
            if (sensor.has_presence() && sensor.presence()) {
                std::cout << "Bewegung erkannt, ";
            }
            if (sensor.has_temperature()) {
                std::cout << "Temperatur: " << sensor.temperature() << "°";
            }
            std::cout << std::endl;
        }
        return absl::OkStatus();
    });
    /*
    auto handler = std::make_shared<hueplusplus::LinHttpHandler>();

    if (config.hue().bridge_size() == 0) {
        std::cerr << "No bridges found\n";
        return 1;
    } else {
        std::cout << config.hue().bridge_size() << " bridges found:" << std::endl;
        for (const auto& bridgecfg : config.hue().bridge()) {
            hueplusplus::Bridge bridge(bridgecfg.host(), bridgecfg.port(), bridgecfg.username(), handler, bridgecfg.client_key());
            std::cout << bridge.getBridgeIP() << ":" << bridge.getBridgePort() << "  ("
                << bridge.lights().getAll().size() << " lights, "
                << bridge.sensors().getAll().size() << " sensors)"
                << std::endl;

            for (const auto& group : bridge.groups().getAll()) {
                if (group.getType() == "Room") {
                    std::cout << "- " << group.getName() << std::endl;

                    for (int lightId: group.getLightIds()) {
                        hueplusplus::Light light = bridge.lights().get(lightId);
                        std::cout << "  - " << light.getName() << ": " << (light.isOn() ? "on" : "off") << std::endl;
                    }
                }
            }
            std::cout << "- sensors:" << std::endl;
            for (const auto& sensor : bridge.sensors().getAll()) {
                std::cout << "  - " << sensor.getUId() << " " << sensor.getName() << ": " << sensor.getType() << std::endl;
            }

            std::cout << "- temperatures:" << std::endl;
            for (const auto& sensor : bridge.sensors().getAllByType<hueplusplus::sensors::ZLLTemperature>()) {
                std::cout << "  - " << sensor.getName() << ": " << (sensor.getTemperature()/100.0) << "°" << std::endl;
            }
        }
    }
     */
    return 0;
}