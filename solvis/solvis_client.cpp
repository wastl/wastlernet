//
// Created by wastl on 02.04.23.
//

#include "solvis_client.h"

#include <absl/strings/str_cat.h>
#include <glog/logging.h>
#include <modbus/modbus.h>
#include <modbus/modbus-tcp.h>

absl::Status solvis::query(const absl::string_view host, int port,
                           const std::function<void(const SolvisData &)> &handler) {
    std::string host_(host);

    modbus_t *ctx;

    ctx = modbus_new_tcp(host_.c_str(), port);
    if (ctx == nullptr) {
        LOG(ERROR) << "Unable to allocate libmodbus context";
        return absl::InternalError("Unable to allocate libmodbus context");
    }

    if (modbus_connect(ctx) == -1) {
        LOG(ERROR) <<"Connection failed: " << modbus_strerror(errno);
        modbus_free(ctx);
        return absl::InternalError(absl::StrCat("Connection failed: ",modbus_strerror(errno)));
    }

    uint16_t tab_reg[64];
    int rc = modbus_read_registers(ctx, 33024, 18, tab_reg);
    if (rc == -1) {
        LOG(ERROR) << "Error reading register: " << modbus_strerror(errno);
        return absl::InternalError(absl::StrCat("Error reading register: ", modbus_strerror(errno)));
    }

    if (rc < 17) {
        LOG(ERROR) << "Could not retrieve all registers" << std::endl;
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

    data.set_solar_kollektor(tab_reg[7] / 10.0);
    data.set_solar_vorlauf(tab_reg[4] / 10.0);
    data.set_solar_ruecklauf(tab_reg[5] / 10.0);
    data.set_solar_waermetauscher(tab_reg[6] / 10.0);
    data.set_solar_volumenstrom(tab_reg[16]);
    data.set_solar_leistung((tab_reg[4] - tab_reg[5]) * tab_reg[16] / 8600.0);

    data.set_kessel(tab_reg[13] / 10.0);

    data.set_vorlauf_heizkreis1(tab_reg[11] / 10.0);
    data.set_vorlauf_heizkreis2(tab_reg[12] / 10.0);
    data.set_vorlauf_heizkreis3(tab_reg[15] / 10.0);

    modbus_close(ctx);
    modbus_free(ctx);

    handler(data);

    return absl::OkStatus();
}
