//
// Created by wastl on 24.11.25.
//

#ifndef WASTLERNET_SHELLY_MODULE_H
#define WASTLERNET_SHELLY_MODULE_H
#include "base/module.h"
#include "shelly/shelly.pb.h"
#include "shelly/shelly_timescaledb.h"
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
                     const std::string& mqtt_address);

        // Convenience constructor: create a `ShellyWriter` internally for TimescaleDB writes.
        ShellyModule(const TimescaleDB& config,
                     StateCache* current_state,
                     const std::string& mqtt_address)
                : Module(config, new ShellyWriter, current_state) {
            // host_/port_ will be set in the out-of-line delegating constructor definition
            // but we still need to parse the address here since we cannot delegate to another
            // constructor due to base-class initialization above.
            ParseMqttAddress(mqtt_address, &host_, &port_);
        }

        std::string Name() override {
            return "shelly";
        }

        void Start() override;

        void Abort() override;

        void Wait() override;

        // Called for every incoming MQTT message on topics matching "shelly/#".
        // Default implementation just logs the message. Override in subclasses if needed.
        virtual absl::Status HandleMqttMessage(const std::string& topic, const web::json::value& payload_json);

    private:
        // MQTT helpers and state
        static void OnConnect(struct mosquitto* m, void* obj, int rc);
        static void OnMessage(struct mosquitto* m, void* obj, const struct mosquitto_message* msg);

        // Helper to parse MQTT address (host[:port])
        static void ParseMqttAddress(const std::string& mqtt_address, std::string* host, int* port);

        std::string host_;
        int port_ = 0;
        struct mosquitto* mqtt_{nullptr};
        std::atomic<bool> running_{false};
    };
}
#endif //WASTLERNET_SHELLY_MODULE_H
