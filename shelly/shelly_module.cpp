//
// Created by wastl on 24.11.25.
//

#include "shelly_module.h"

#include <glog/logging.h>
#include <absl/strings/str_split.h>
#include <absl/strings/str_cat.h>
#include <vector>

using web::json::value;
using utility::conversions::to_string_t;

namespace wastlernet::shelly {
    namespace {
        bool check_fields(const value& json, std::initializer_list<const char*> fields) {
            return std::all_of(fields.begin(), fields.end(), [&json](const char* f) { return json.has_field(f); });
        }

        bool check_method(const value& json, const char* method) { return json.at("method").as_string() == method; }
    }

    void ShellyModule::ParseMqttAddress(const std::string& mqtt_address, std::string* host, int* port) {
        *port = 1883;
        *host = mqtt_address;
        // very small parser: accept "host" or "host:port"
        // Use concrete container to access size/index; using `auto` would yield a Splitter view.
        std::vector<std::string> parts = absl::StrSplit(mqtt_address, ':');
        if (parts.size() == 2) {
            *host = parts[0];
            try {
                *port = std::stoi(parts[1]);
            } catch (...) {
                *port = 1883;
            }
        }
    }

    void ShellyModule::OnConnect(mosquitto* m, void* /*obj*/, int rc) {
        if (rc != 0) {
            LOG(ERROR) << "Shelly MQTT connect failed with code " << rc;
            return;
        }
        LOG(INFO) << "Shelly MQTT connected. Subscribing to topics 'shelly/#'";
        int mid = 0;
        int res = mosquitto_subscribe(m, &mid, "shelly/#", 0);
        if (res != MOSQ_ERR_SUCCESS) {
            LOG(ERROR) << "Shelly MQTT subscribe failed: " << mosquitto_strerror(res);
        }
    }

    void ShellyModule::OnMessage(mosquitto* /*m*/, void* obj, const mosquitto_message* msg) {
        auto self = static_cast<ShellyModule*>(obj);
        if (!self)
            return;
        try {
            std::string topic = msg && msg->topic ? msg->topic : "";
            std::string payload;
            if (msg && msg->payload && msg->payloadlen > 0) {
                payload.assign(static_cast<const char*>(msg->payload), static_cast<size_t>(msg->payloadlen));
            }
            // Parse JSON using cpprestsdk
            value json = value::parse(to_string_t(payload));
            auto st = self->HandleMqttMessage(topic, json);
            if (!st.ok()) {
                LOG(ERROR) << "Shelly MQTT message handling failed: " << st.message();
            }
        } catch (const std::exception& e) {
            LOG(ERROR) << "Shelly MQTT message handling error: " << e.what();
        }
    }

    ShellyModule::ShellyModule(const TimescaleDB& config, timescaledb::TimescaleWriter<ShellyData>* writer,
                               StateCache* current_state, const std::string& mqtt_address) : Module(
        config, writer, current_state) {
        ParseMqttAddress(mqtt_address, &host_, &port_);
    }

    void ShellyModule::Start() {
        if (running_.exchange(true)) {
            return; // already running
        }

        mosquitto_lib_init();
        mqtt_ = mosquitto_new("wastlernet-shelly", true, this);
        if (!mqtt_) {
            LOG(ERROR) << "Failed to create mosquitto client";
            running_ = false;
            return;
        }

        mosquitto_connect_callback_set(mqtt_, &ShellyModule::OnConnect);
        mosquitto_message_callback_set(mqtt_, &ShellyModule::OnMessage);

        int keepalive = 60;
        int rc = mosquitto_connect(mqtt_, host_.c_str(), port_, keepalive);
        if (rc != MOSQ_ERR_SUCCESS) {
            LOG(ERROR) << "Shelly MQTT connection failed to " << host_ << ":" << port_
                << " - " << mosquitto_strerror(rc);
            mosquitto_destroy(mqtt_);
            mqtt_ = nullptr;
            running_ = false;
            mosquitto_lib_cleanup();
            return;
        }

        // Start the network loop thread
        rc = mosquitto_loop_start(mqtt_);
        if (rc != MOSQ_ERR_SUCCESS) {
            LOG(ERROR) << "Shelly MQTT loop_start failed: " << mosquitto_strerror(rc);
            mosquitto_disconnect(mqtt_);
            mosquitto_destroy(mqtt_);
            mqtt_ = nullptr;
            running_ = false;
            mosquitto_lib_cleanup();
            return;
        }
        LOG(INFO) << "Shelly MQTT client started, connecting to " << host_ << ":" << port_;
    }

    void ShellyModule::Abort() {
        if (!running_.exchange(false)) {
            return;
        }
        if (mqtt_) {
            mosquitto_disconnect(mqtt_);
            mosquitto_loop_stop(mqtt_, true);
            mosquitto_destroy(mqtt_);
            mqtt_ = nullptr;
        }
        mosquitto_lib_cleanup();
    }

    void ShellyModule::Wait() {
        // No explicit worker thread to join here, mosquitto_loop_start manages its own thread
    }

    absl::Status ShellyModule::HandleMqttMessage(const std::string& topic, const web::json::value& payload_json) {
        try {
            // Default: just log the message. Modules may override to transform into ShellyData and store.
            LOG(INFO) << absl::StrCat("Shelly MQTT message on ", topic, ": ",
                                      utility::conversions::to_utf8string(payload_json.serialize()));

            bool has_data = false;

            ShellyData data;
            std::vector<std::string> dsts = absl::StrSplit(utility::conversions::to_utf8string(topic), '/');

            if (dsts.size() > 2) {
                data.set_device_name(absl::StrCat(dsts[1], "-", dsts[2]));
            } else {
                // fallback
                data.set_device_name(topic);
            }

            if (check_fields(payload_json, {"method", "params"}) &&
                (check_method(payload_json, "NotifyFullStatus") || check_method(payload_json, "NotifyStatus"))) {

                auto params = payload_json.at("params");

                // Thermometer
                if (params.has_field("temperature:0")) {
                    data.mutable_temperature_data()->set_temperature(params.at("temperature:0").at("tC").as_double());
                    has_data = true;
                }
                if (params.has_field("humidity:0")) {
                    data.mutable_temperature_data()->set_humidity(params.at("humidity:0").at("rh").as_double());
                    has_data = true;
                }
                if (params.has_field("illuminance:0")) {
                    data.mutable_light_data()->set_lux(params.at("illuminance:0").at("lux").as_integer());
                    data.mutable_light_data()->set_illumination(
                        params.at("illuminance:0").at("illumination").as_string());
                    has_data = true;
                }


            }

            if (check_fields(payload_json, {"apower", "current", "voltage", "freq"})) {
                EnergyData* energy = data.mutable_energy_data();
                energy->set_power(payload_json.at("apower").as_double());
                energy->set_current(payload_json.at("current").as_double());
                energy->set_voltage(payload_json.at("voltage").as_double());
                energy->set_frequency(payload_json.at("freq").as_double());
                has_data = true;
            }

            if (has_data)
                return Update(data);

            return absl::OkStatus();
        } catch (const std::exception& e) {
            LOG(ERROR) << "Error logging Shelly MQTT message: " << e.what();
            return absl::InternalError(e.what());
        }
    }
} // namespace wastlernet::shelly
