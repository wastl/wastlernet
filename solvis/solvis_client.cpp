//
// Created by wastl on 02.04.23.
//

#include "solvis_client.h"
#include "include/utility.h"

#include <fstream>
#include <limits.h>
#include <absl/strings/str_cat.h>
#include <glog/logging.h>
#include <modbus/modbus.h>
#include <modbus/modbus-tcp.h>

#define LOGS(level) LOG(level) << "[solvis] "

int uint2int(uint16_t u) {
    if (u <= (unsigned int)INT_MAX)
        return (int)u;
    else
        return -(int)~u - 1;
}

absl::Status solvis::query(const absl::string_view host, int port,
                           const std::function<void(const SolvisData &)> &handler) {
    std::string host_(host);

    modbus_t *ctx;

    LOGS(INFO) << "Establishing Modbus connection";

    ctx = modbus_new_tcp(host_.c_str(), port);
    if (ctx == nullptr) {
        LOGS(ERROR) << "Unable to allocate libmodbus context";
        return absl::InternalError("Unable to allocate libmodbus context");
    }

    auto c_st = wastlernet::retry_with_backoff([ctx]() {
        int rc = modbus_connect(ctx);
        if (rc == -1) {
            return absl::InternalError(absl::StrCat("Connection failed: ",modbus_strerror(errno)));
        }
        return absl::OkStatus();
    }, 3);

    if (!c_st.ok()) {
        LOGS(ERROR) << c_st;
        modbus_free(ctx);
        return c_st;
    }

    LOGS(INFO) << "Reading Modbus registers";

    uint16_t tab_reg[64];
    int rc1 = modbus_read_registers(ctx, 33024, 18, tab_reg);
    int rc2 = modbus_read_registers(ctx, 33536, 5, &tab_reg[20]);
    int rc3 = modbus_read_registers(ctx, 33280, 20, &tab_reg[32]);

    LOGS(INFO) << "Closing Modbus connection";

    modbus_close(ctx);
    modbus_free(ctx);

    if (rc1 == -1 || rc2 == -1 || rc3 == -1) {
        LOGS(ERROR) << "Error reading register: " << modbus_strerror(errno);
        return absl::InternalError(absl::StrCat("Error reading register: ", modbus_strerror(errno)));
    }

    if (rc1 < 17) {
        LOGS(ERROR) << "Could not retrieve all registers" << std::endl;
        return absl::InternalError("Could not retrieve all registers");
    }
    if (rc2 < 5) {
        LOGS(ERROR) << "Could not retrieve all registers" << std::endl;
        return absl::InternalError("Could not retrieve all registers");
    }

    SolvisData data;
    data.set_speicher_oben(tab_reg[0] / 10.0);
    data.set_heizungspuffer_oben(tab_reg[3] / 10.0);
    data.set_heizungspuffer_unten(tab_reg[8] / 10.0);
    data.set_speicher_unten(tab_reg[2] / 10.0);

    data.set_warmwasser(tab_reg[1] / 10.0);
    data.set_kaltwasser(tab_reg[14] / 10.0);
    data.set_zirkulation(tab_reg[10] / 10.0);
    data.set_durchfluss(tab_reg[17]);

    data.set_solar_kollektor(uint2int(tab_reg[7]) / 10.0);
    data.set_solar_vorlauf(tab_reg[4] / 10.0);
    data.set_solar_ruecklauf(tab_reg[5] / 10.0);
    data.set_solar_waermetauscher(tab_reg[6] / 10.0);
    data.set_solar_volumenstrom(tab_reg[16]);
    data.set_solar_leistung((tab_reg[4] - tab_reg[5]) * tab_reg[16] / 8600.0);

    data.set_kessel(tab_reg[13] / 10.0);

    data.set_vorlauf_heizkreis1(tab_reg[11] / 10.0);
    data.set_vorlauf_heizkreis2(tab_reg[12] / 10.0);
    data.set_vorlauf_heizkreis3(tab_reg[15] / 10.0);
    data.set_pumpe_heizkreis1(tab_reg[32+2] > 0);
    data.set_pumpe_heizkreis2(tab_reg[32+3] > 0);
    data.set_pumpe_heizkreis3(tab_reg[32+4] > 0);

    //data.set_kessel_leistung(tab_reg[23] / 1000.0);
    data.set_kessel_ladepumpe(tab_reg[32+12] > 0);
    data.set_kessel_brenner(tab_reg[32+11] > 0);

    // 26 kW maximale Leistung * aktuelle Leistung Ladepumpe in % / 100
    data.set_kessel_leistung(0.26 * tab_reg[46+3] * 0.1);

    for (int i = 0; i<14; i++) {
        data.add_ausgang(tab_reg[32+i] * 0.5);
    }

    for (int i=0; i<6; i++) {
        data.add_analog_out(tab_reg[46+i] * 0.1);
    }

    std::ofstream dbg("/tmp/solvis.textpb");
    dbg << data.DebugString();
    dbg.close();

    LOGS(INFO) << "running handler";

    handler(data);

    return absl::OkStatus();
}
