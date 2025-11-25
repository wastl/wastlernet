//
// Created by wastl on 24.11.25.
//

#ifndef WASTLERNET_SHELLY_MODULE_H
#define WASTLERNET_SHELLY_MODULE_H
#include "base/module.h"
#include "shelly/shelly.pb.h"
#include <string>
#include <atomic>
#include <mosquitto.h>
#include <cpprest/json.h>

namespace wastlernet::shelly {
    class ShellyModule : public Module<ShellyData> {
    public:
        // Construct with TimescaleDB config and MQTT broker address (e.g. "localhost" or "localhost:1883").
        ShellyModule(const TimescaleDB& config,
                     timescaledb::TimescaleWriter<ShellyData>* writer,
                     StateCache* current_state,
                     const std::string& mqtt_address)
            : Module(config, writer, current_state), mqtt_address_(mqtt_address) {}

        std::string Name() override {
            return "shelly";
        }

        void Start() override;

        void Abort() override;

        void Wait() override;

        // Called for every incoming MQTT message on topics matching "shelly/#".
        // Default implementation just logs the message. Override in subclasses if needed.
        virtual void HandleMqttMessage(const std::string& topic, const web::json::value& payload_json);

    private:
        // MQTT helpers and state
        static void OnConnect(struct mosquitto* m, void* obj, int rc);
        static void OnMessage(struct mosquitto* m, void* obj, const struct mosquitto_message* msg);

        // Parse the provided mqtt_address_ into host and port.
        void ParseMqttAddress(std::string* host, int* port) const;

        std::string mqtt_address_;
        struct mosquitto* mqtt_{nullptr};
        std::atomic<bool> running_{false};
    };
}
#endif //WASTLERNET_SHELLY_MODULE_H
