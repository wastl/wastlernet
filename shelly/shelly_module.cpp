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

void ShellyModule::ParseMqttAddress(std::string* host, int* port) const {
    *port = 1883;
    *host = mqtt_address_;
    // very small parser: accept "host" or "host:port"
    // Use concrete container to access size/index; using `auto` would yield a Splitter view.
    std::vector<std::string> parts = absl::StrSplit(mqtt_address_, ':');
    if (parts.size() == 2) {
        *host = parts[0];
        try {
            *port = std::stoi(parts[1]);
        } catch (...) {
            *port = 1883;
        }
    }
}

void ShellyModule::OnConnect(struct mosquitto* m, void* obj, int rc) {
    auto self = static_cast<ShellyModule*>(obj);
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

void ShellyModule::OnMessage(struct mosquitto* m, void* obj, const struct mosquitto_message* msg) {
    auto self = static_cast<ShellyModule*>(obj);
    if (!self) return;
    try {
        std::string topic = msg && msg->topic ? msg->topic : "";
        std::string payload;
        if (msg && msg->payload && msg->payloadlen > 0) {
            payload.assign(static_cast<const char*>(msg->payload), static_cast<size_t>(msg->payloadlen));
        }
        // Parse JSON using cpprestsdk
        value json = value::parse(to_string_t(payload));
        self->HandleMqttMessage(topic, json);
    } catch (const std::exception& e) {
        LOG(ERROR) << "Shelly MQTT message handling error: " << e.what();
    }
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

    std::string host;
    int port;
    ParseMqttAddress(&host, &port);

    int keepalive = 60;
    int rc = mosquitto_connect(mqtt_, host.c_str(), port, keepalive);
    if (rc != MOSQ_ERR_SUCCESS) {
        LOG(ERROR) << "Shelly MQTT connection failed to " << host << ":" << port
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
    LOG(INFO) << "Shelly MQTT client started, connecting to " << host << ":" << port;
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

void ShellyModule::HandleMqttMessage(const std::string& topic, const web::json::value& payload_json) {
    try {
        // Default: just log the message. Modules may override to transform into ShellyData and store.
        LOG(INFO) << absl::StrCat("Shelly MQTT message on ", topic, ": ",
                                  utility::conversions::to_utf8string(payload_json.serialize()));
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error logging Shelly MQTT message: " << e.what();
    }
}

} // namespace wastlernet::shelly
