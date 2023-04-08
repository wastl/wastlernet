//
// Created by wastl on 02.04.23.
//

#include "solvis_client.h"

#include <glog/logging.h>
#include <modbus/modbus.h>
#include <modbus/modbus-tcp.h>

void solvis::query(const absl::string_view host, int port,
                   const std::function<void(const SolvisData &)> &handler) {
    std::cout << "Solvis QUERY: " << host << ":" << port << std::endl;

    std::string host_(host);

    modbus_t *ctx;

    ctx = modbus_new_tcp(host_.c_str(), port);
    if (ctx == nullptr) {
        std::cerr <<"Unable to allocate libmodbus context" << std::endl;
        return;
    }

    if (modbus_connect(ctx) == -1) {
        std::cerr <<"Connection failed: " << modbus_strerror(errno) << std::endl;
        modbus_free(ctx);
        return;
    }

    uint16_t tab_reg[64];
    int rc = modbus_read_registers(ctx, 33024, 18, tab_reg);
    if (rc == -1) {
        std::cerr << "Error reading register: " << modbus_strerror(errno) << std::endl;
        return;
    }

    if (rc < 17) {
        std::cerr << "Could not retrieve all registers" << std::endl;
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
    // 0: Speicher oben (S1)
    // 3: Heizungspuffer oben (S4)
    // 8: Heizungspuffer unten (S9)
    // 2: Speicher unten (S4)
    // 1: Warmwasser (S2)
    // 14: Kaltwasser (S15)
    // 10: Zirkulation

    // 9: Aussentemperatur
    // 11: Heizkreis 1
    // 12: Heizkreis 2
    // 13: Kessel

/*
S1 Speicher oben
S3 Speicherreferenz
S4 Heizungs-Puffer oben
S5 Solar-Vorlauf 2
S6 Solar-Rücklauf 2
S7 Solar-Vorlauf 1
S8 Kollektor
S9 Heizungs-Puffer unten (SolvisBen Solo und SolvisMax)
S17 Solarvolumenstrom
*/
/*
    for (int i=0; i < rc; i++) {
        printf("reg[%d]=%d (0x%X)\n", i, tab_reg[i], tab_reg[i]);
    }
*/


    modbus_close(ctx);
    modbus_free(ctx);

    handler(data);
}
