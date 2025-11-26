// Simple standalone debug client for ShellyModule
// Usage: shelly_debug <mqtt-address>

#include <csignal>
#include <atomic>
#include <iostream>
#include <glog/logging.h>

#include "config/config.pb.h"
#include "base/module.h"
#include "shelly/shelly_module.h"

using wastlernet::StateCache;
using wastlernet::TimescaleDB;

namespace wastlernet::shelly {

// A thin wrapper that dumps received MQTT messages to stdout.
class ShellyDebugModule : public ShellyModule {
public:
    ShellyDebugModule(const TimescaleDB& cfg,
                      timescaledb::TimescaleWriter<ShellyData>* writer,
                      StateCache* cache,
                      const std::string& mqtt_addr)
        : ShellyModule(cfg, writer, cache, mqtt_addr) {}



protected:
    absl::Status Update(const ShellyData& data) override {
        std::cout << "[Shelly] " << data.Utf8DebugString() << std::endl;
        return absl::OkStatus();
    }
};

} // namespace wastlernet::shelly

static std::atomic<bool> g_stop{false};

static void handle_signal(int) {
    g_stop = true;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: shelly_debug <mqtt-address>\n"
                  << "Example: shelly_debug localhost:1883" << std::endl;
        return 1;
    }

    // Initialize logging
    google::InitGoogleLogging(argv[0]);

    // Setup signal handlers for graceful shutdown
    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

    std::string mqtt_address = argv[1];

    // Minimal, mostly-unused TimescaleDB config; we won't call Init() or write.
    TimescaleDB ts_cfg; // leaves fields default/empty
    StateCache cache;

    // We don't need DB writes; pass a null writer pointer. The debug tool never calls Init/Update.
    wastlernet::shelly::ShellyDebugModule module(ts_cfg, /*writer=*/nullptr, &cache, mqtt_address);

    // Start MQTT client and run until interrupted.
    module.Start();

    while (!g_stop.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    module.Abort();
    module.Wait();

    return 0;
}
