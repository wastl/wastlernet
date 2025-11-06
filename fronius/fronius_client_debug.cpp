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
DEFINE_string(slave, "http://127.0.0.1", "Fronius Gen24 base URL (e.g., http://inverter)");

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

    fronius::FroniusData data;

    // Construct a minimal HTTP client and perform a single query
    fronius::FroniusPowerFlowClient power_flow_client(FLAGS_host);
    FAIL_IF_ERROR(power_flow_client.Init());
    FAIL_IF_ERROR(power_flow_client.Query([&](const fronius::Leistung &l, const fronius::Quellen& q) {
        *data.mutable_leistung() = l;
    }));

    if (!FLAGS_slave.empty()) {
        fronius::FroniusPowerFlowClient power_flow_slave(FLAGS_slave);
        FAIL_IF_ERROR(power_flow_slave.Init());
        FAIL_IF_ERROR(power_flow_slave.Query([&](const fronius::Leistung &l, const fronius::Quellen& q) {
            data.mutable_leistung()->set_pv_leistung(data.leistung().pv_leistung() + l.pv_leistung());
        }));
    }

    fronius::FroniusEnergyMeterClient energy_meter_client(FLAGS_host);
    FAIL_IF_ERROR(energy_meter_client.Init());
    FAIL_IF_ERROR(energy_meter_client.Query([&](double consumption_watts) {
        data.mutable_leistung()->set_hausverbrauch(consumption_watts);
    }));

    fronius::FroniusBatteryClient battery_client(FLAGS_host);
    FAIL_IF_ERROR(battery_client.Init());
    FAIL_IF_ERROR(battery_client.Query([&](const fronius::Batterie& b) {
        *data.mutable_batterie() = b;
    }));

    fronius::Quellen quellen;
    if (data.leistung().batterie_leistung() > 0) {
        quellen.set_laden(0);
        quellen.set_entladen(data.leistung().batterie_leistung());
    } else {
        quellen.set_laden(-data.leistung().batterie_leistung());
        quellen.set_entladen(0);
    }

    if (data.leistung().netz_leistung() < 0) {
        quellen.set_einspeisung(-data.leistung().netz_leistung());
        quellen.set_bezug(0);
    } else {
        quellen.set_einspeisung(0);
        quellen.set_bezug(data.leistung().netz_leistung());
    }
    *data.mutable_quellen() = quellen;

    // Fix up consumption
    double consumption = data.leistung().pv_leistung()+data.leistung().batterie_leistung()+data.leistung().netz_leistung();

    std::cout << "Fronius Data: " << data.DebugString() << std::endl;

    std::cout << "Consumption: " << consumption << std::endl;

    return 0;
}
