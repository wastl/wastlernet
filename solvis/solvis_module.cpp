//
// Created by wastl on 07.04.23.
//

#include "solvis_module.h"

#include <fstream>
#include <limits.h>
#include <absl/strings/str_cat.h>
#include <glog/logging.h>

#include "base/metrics.h"

#define LOGS(level) LOG(level) << "[solvis] "

int16_t uint2int(uint16_t u) {
    if (u <= (uint16_t) INT16_MAX)
        return (int16_t) u;
    else
        return -(int16_t) ~u - 1;
}

absl::Status solvis::SolvisModule::Query(std::function<absl::Status(const solvis::SolvisData &)> handler) {
    auto start_time = std::chrono::high_resolution_clock::now();
    try {
        auto st = conn_->Execute([handler](modbus_t *ctx) {
            LOGS(INFO) << "Reading Modbus registers";

            uint16_t tab_reg[64];
            int rc1 = modbus_read_registers(ctx, 33024, 18, tab_reg);
            int rc2 = modbus_read_registers(ctx, 33536, 5, &tab_reg[20]);
            int rc3 = modbus_read_registers(ctx, 33280, 20, &tab_reg[32]);

            if (rc1 == -1 || rc2 == -1 || rc3 == -1) {
                wastlernet::metrics::WastlernetMetrics::GetInstance().solvis_error_counter.Increment();
                LOGS(ERROR) << "Error reading register: " << modbus_strerror(errno);
                return absl::InternalError(absl::StrCat("Error reading register: ", modbus_strerror(errno)));
            }

            if (rc1 < 17) {
                wastlernet::metrics::WastlernetMetrics::GetInstance().solvis_error_counter.Increment();
                LOGS(ERROR) << "Could not retrieve all registers" << std::endl;
                return absl::InternalError("Could not retrieve all registers");
            }
            if (rc2 < 5) {
                wastlernet::metrics::WastlernetMetrics::GetInstance().solvis_error_counter.Increment();
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
            data.set_pumpe_heizkreis1(tab_reg[32 + 2] > 0);
            data.set_pumpe_heizkreis2(tab_reg[32 + 3] > 0);
            data.set_pumpe_heizkreis3(tab_reg[32 + 4] > 0);

            //data.set_kessel_leistung(tab_reg[23] / 1000.0);
            data.set_kessel_ladepumpe(tab_reg[32 + 12] > 0);
            data.set_kessel_brenner(tab_reg[32 + 11] > 0);

            // 26 kW maximale Leistung * aktuelle Leistung Ladepumpe in % / 100
            data.set_kessel_leistung(0.26 * tab_reg[46 + 3] * 0.1);

            for (int i = 0; i < 14; i++) {
                data.add_ausgang(tab_reg[32 + i] * 0.5);
            }

            for (int i = 0; i < 6; i++) {
                data.add_analog_out(tab_reg[46 + i] * 0.1);
            }

            std::ofstream dbg("/tmp/solvis.textpb");
            dbg << data.DebugString();
            dbg.close();

            LOGS(INFO) << "running handler";

            return handler(data);
        });

        if (st.ok()) {
            wastlernet::metrics::WastlernetMetrics::GetInstance().solvis_query_counter.Increment();

            auto end_time = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> duration = end_time - start_time;
            wastlernet::metrics::WastlernetMetrics::GetInstance().solvis_duration_ms.Observe(
                std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
        } else {
            wastlernet::metrics::WastlernetMetrics::GetInstance().solvis_error_counter.Increment();
        }

        return st;
    } catch (std::exception const &e) {
        LOG(ERROR) << "Error querying Solvis controller: " << e.what();
        wastlernet::metrics::WastlernetMetrics::GetInstance().solvis_error_counter.Increment();
        return absl::InternalError(e.what());
    }
}
