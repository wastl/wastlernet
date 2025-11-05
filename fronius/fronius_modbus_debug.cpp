//
// Simple CLI to query Fronius via Solar API JSON and dump FroniusData
//

#include <iostream>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <absl/status/status.h>

#include "fronius/fronius.pb.h"
#include "fronius/fronius_client.h"

DEFINE_string(host, "http://127.0.0.1", "Fronius Gen24 base URL (e.g., http://inverter)");

#define FAIL_IF_ERROR(call) {                       \
    auto call_st = call;                            \
    if (!call_st.ok()) {                            \
        LOG(ERROR) << "Call failed: " << call_st;   \
        return 1;                                   \
    }}


int main(int argc, char **argv) {
    // Init gflags and glog
    gflags::SetUsageMessage("Usage: fronius_client_debug --host=<base-url>");
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);

    if (FLAGS_host.empty()) {
        std::cerr << "--host is required" << std::endl;
        return 2;
    }

    // Construct a minimal HTTP client and perform a single query
    fronius::FroniusPowerFlowClient power_flow_client(FLAGS_host);
    FAIL_IF_ERROR(power_flow_client.Init());
    FAIL_IF_ERROR(power_flow_client.Query([](const fronius::Leistung &l, const fronius::Quellen& q) {
        std::cout << "Leistung: " << l.DebugString() << std::endl;
        std::cout << "Quellen: " << q.DebugString() << std::endl;
    }));

    fronius::FroniusBatteryClient battery_client(FLAGS_host);
    FAIL_IF_ERROR(battery_client.Init());
    FAIL_IF_ERROR(battery_client.Query([](const fronius::Batterie& b) {
        std::cout << "Batterie: " << b.DebugString() << std::endl;
    }));

    return 0;
}
