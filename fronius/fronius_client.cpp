//
// Fronius Solar API JSON client
//
#include <cpprest/uri.h>
#include <cpprest/http_msg.h>
#include <cpprest/http_client.h>
#include <cpprest/json.h>
#include <glog/logging.h>
#include <absl/strings/str_cat.h>
#include <chrono>

#include "base/metrics.h"
#include "fronius_client.h"


#define LOGF(level) LOG(level) << "[fronius] "

namespace fronius {
    using namespace web; // Common features like URIs.
    using namespace web::http; // Common HTTP functionality
    using namespace web::http::client; // HTTP client features
    using namespace web::json; // JSON library

    absl::Status FroniusPowerFlowClient::Query(const std::function<void(const Leistung&, const Quellen&)>& handler) {
        auto start_time = std::chrono::high_resolution_clock::now();

        auto st = Execute([&](const http_response& response) {
            if (response.status_code() != status_codes::OK) {
                LOGF(ERROR) << "Fronius query failed: " << response.reason_phrase();
                return absl::InternalError(absl::StrCat("Fronius query failed: ", response.reason_phrase()));
            }

            value result = response.extract_json(true).get();

            // Expected structure: { "Body": { "Data": { "Site": {...}, "Inverters": {...}, "Storage": {...} } } }
            auto bodyIt = result.as_object().find(U("Body"));
            if (bodyIt == result.as_object().end()) {
                LOGF(ERROR) << "Unexpected JSON: missing Body: " << result.serialize();
                return absl::InternalError("Unexpected Fronius JSON (no Body)");
            }
            auto dataIt = bodyIt->second.as_object().find(U("Data"));
            if (dataIt == bodyIt->second.as_object().end()) {
                LOGF(ERROR) << "Unexpected JSON: missing Body.Data: " << result.serialize();
                return absl::InternalError("Unexpected Fronius JSON (no Body.Data)");
            }

            const value& Data = dataIt->second;

            if (!Data.has_field(U("Site"))) {
                LOGF(ERROR) << "Unexpected JSON: missing Body.Data.Site: " << result.serialize();
                return absl::InternalError("Unexpected Fronius JSON (no Body.Data.Site)");
            }

            Leistung data;

            const auto& Site = Data.at(U("Site"));
            auto getd = [&](const char* k) -> double {
                try {
                    auto sk = utility::conversions::to_string_t(k);
                    if (Site.has_field(sk))
                        return Site.at(sk).as_double();
                } catch (...) {
                }
                return 0.0;
            };
            double p_load = getd("P_Load"); // W, positive consumption
            double p_pv = getd("P_PV"); // W, PV production
            double p_grid = getd("P_Grid"); // W, positive import, negative export
            double p_akku = getd("P_Akku"); // W, positive discharge, negative charge

            data.set_hausverbrauch(-p_load);
            data.set_pv_leistung(p_pv);
            data.set_netz_leistung(p_grid);
            data.set_batterie_leistung(p_akku);

            Quellen quellen;
            if (data.batterie_leistung() > 0) {
                quellen.set_laden(0);
                quellen.set_entladen(data.batterie_leistung());
            } else {
                quellen.set_laden(-data.batterie_leistung());
                quellen.set_entladen(0);
            }

            if (data.netz_leistung() < 0) {
                quellen.set_einspeisung(-data.netz_leistung());
                quellen.set_bezug(0);
            } else {
                quellen.set_einspeisung(0);
                quellen.set_bezug(data.netz_leistung());
            }

            handler(data, quellen);
            return absl::OkStatus();
        });

        if (st.ok()) {
            wastlernet::metrics::WastlernetMetrics::GetInstance().fronius_query_counter.Increment();
            auto end_time = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> duration = end_time - start_time;
            wastlernet::metrics::WastlernetMetrics::GetInstance().fronius_duration_ms.Observe(
                std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
        } else {
            wastlernet::metrics::WastlernetMetrics::GetInstance().fronius_error_counter.Increment();
        }

        return st;
    }

    absl::Status FroniusBatteryClient::Query(const std::function<void(const Batterie&)>& handler) {
        auto start_time = std::chrono::high_resolution_clock::now();

        auto st = Execute([&](const http_response& response) {
            if (response.status_code() != status_codes::OK) {
                LOGF(ERROR) << "Fronius query failed: " << response.reason_phrase();
                return absl::InternalError(absl::StrCat("Fronius query failed: ", response.reason_phrase()));
            }

            value result = response.extract_json(true).get();

            // Expected structure: { "Body": { "Data": { "Site": {...}, "Inverters": {...}, "Storage": {...} } } }
            auto bodyIt = result.as_object().find(U("Body"));
            if (bodyIt == result.as_object().end()) {
                LOGF(ERROR) << "Unexpected JSON: missing Body: " << result.serialize();
                return absl::InternalError("Unexpected Fronius JSON (no Body)");
            }
            auto dataIt = bodyIt->second.as_object().find(U("Data"));
            if (dataIt == bodyIt->second.as_object().end()) {
                LOGF(ERROR) << "Unexpected JSON: missing Body.Data: " << result.serialize();
                return absl::InternalError("Unexpected Fronius JSON (no Body.Data)");
            }

            auto zeroIt = dataIt->second.as_object().find(U("0"));
            if (zeroIt == dataIt->second.as_object().end()) {
                LOGF(ERROR) << "Unexpected JSON: missing Body.Data.0: " << result.serialize();
                return absl::InternalError("Unexpected Fronius JSON (no Body.Data.0)");
            }


            const value& Data = zeroIt->second;

            if (!Data.has_field(U("Controller"))) {
                LOGF(ERROR) << "Unexpected JSON: missing Body.Data.Site.0.Controller: " << result.serialize();
                return absl::InternalError("Unexpected Fronius JSON (no Body.Data.0.Controller)");
            }

            Batterie data;

            const auto& controller = Data.at(U("Controller"));
            auto getd = [&](const char* k) -> double {
                try {
                    auto sk = utility::conversions::to_string_t(k);
                    if (controller.has_field(sk))
                        return controller.at(sk).as_double();
                } catch (...) {
                }
                return 0.0;
            };
            double p_soc = getd("StateOfCharge_Relative"); // %
            double p_current_dc = getd("Current_DC"); // A
            double p_temp = getd("Temperature_Cell"); // °C
            double p_voltage_dc = getd("Voltage_DC"); // V

            data.set_soc(p_soc);
            data.set_spannung(p_voltage_dc);
            data.set_temperatur(p_temp);
            data.set_leistung(p_current_dc);

            handler(data);
            return absl::OkStatus();
        });

        if (st.ok()) {
            wastlernet::metrics::WastlernetMetrics::GetInstance().fronius_query_counter.Increment();
            auto end_time = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> duration = end_time - start_time;
            wastlernet::metrics::WastlernetMetrics::GetInstance().fronius_duration_ms.Observe(
                std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
        } else {
            wastlernet::metrics::WastlernetMetrics::GetInstance().fronius_error_counter.Increment();
        }

        return st;
    }


    http_client_config FroniusBaseClient::ClientConfig() {
        http_client_config config = HttpConnection::ClientConfig();
        config.set_validate_certificates(false);
        return config;
    }

}
/*
namespace fronius {

absl::Status FroniusClient::Query(const std::function<void(const FroniusData &)> &handler) {
    auto start_time = std::chrono::high_resolution_clock::now();

    auto st = Execute([&](const http_response &response) {
        if (response.status_code() != status_codes::OK) {
            LOGF(ERROR) << "Fronius query failed: " << response.reason_phrase();
            return absl::InternalError(absl::StrCat("Fronius query failed: ", response.reason_phrase()));
        }

        json::value result = response.extract_json(true).get();

        // Expected structure: { "Body": { "Data": { "Site": {...}, "Inverters": {...}, "Storage": {...} } } }
        auto bodyIt = result.as_object().find(U("Body"));
        if (bodyIt == result.as_object().end()) {
            LOGF(ERROR) << "Unexpected JSON: missing Body: " << result.serialize();
            return absl::InternalError("Unexpected Fronius JSON (no Body)");
        }
        auto dataIt = bodyIt->second.as_object().find(U("Data"));
        if (dataIt == bodyIt->second.as_object().end()) {
            LOGF(ERROR) << "Unexpected JSON: missing Body.Data: " << result.serialize();
            return absl::InternalError("Unexpected Fronius JSON (no Body.Data)");
        }

        const json::value &Data = dataIt->second;

        FroniusData out;

        // Site block provides aggregate instantaneous power
        if (Data.has_field(U("Site"))) {
            const auto &Site = Data.at(U("Site"));
            auto getd = [&](const char *k) -> double {
                try {
                    auto sk = utility::conversions::to_string_t(k);
                    if (Site.has_field(sk)) return Site.at(sk).as_double();
                } catch (...) {}
                return 0.0;
            };
            double p_load = getd("P_Load");     // W, positive consumption
            double p_pv = getd("P_PV");         // W, PV production
            double p_grid = getd("P_Grid");     // W, positive import, negative export
            double p_akku = 0.0;                  // W, positive discharge, negative charge
            if (Site.has_field(U("P_Akku"))) {
                try { p_akku = Site.at(U("P_Akku")).as_double(); } catch (...) {}
            } else if (Data.has_field(U("Storage"))) {
                const auto &Storage = Data.at(U("Storage"));
                // Some firmwares expose battery power as negative when charging; try to pick any numeric field
                if (Storage.is_object()) {
                    for (const auto &kv : Storage.as_object()) {
                        const auto &obj = kv.second;
                        if (obj.is_object() && obj.has_field(U("P_Akku"))) {
                            try { p_akku = obj.at(U("P_Akku")).as_double(); } catch (...) {}
                            break;
                        }
                    }
                }
            }

            out.mutable_leistung()->set_hausverbrauch(p_load);
            out.mutable_leistung()->set_pv_leistung(p_pv);
            out.mutable_leistung()->set_netz_leistung(p_grid);
            out.mutable_leistung()->set_batterie_leistung(p_akku);

            // Quellen breakdowns
            if (p_grid < 0) {
                out.mutable_quellen()->set_einspeisung(-p_grid);
                out.mutable_quellen()->set_bezug(0);
            } else {
                out.mutable_quellen()->set_einspeisung(0);
                out.mutable_quellen()->set_bezug(p_grid);
            }
            if (p_akku < 0) {
                out.mutable_quellen()->set_laden(-p_akku);
                out.mutable_quellen()->set_entladen(0);
            } else {
                out.mutable_quellen()->set_laden(0);
                out.mutable_quellen()->set_entladen(p_akku);
            }
        }

        // Battery SoC, temperature etc. Some firmwares provide in Data.Storage or Inverters map with key "1"
        if (Data.has_field(U("Storage"))) {
            const auto &Storage = Data.at(U("Storage"));
            auto fill_from_obj = [&](const json::object &obj){
                if (obj.find(U("StateOfCharge")) != obj.end()) {
                    try { out.mutable_batterie()->set_soc(obj.at(U("StateOfCharge")).as_double()); } catch (...) {}
                }
                if (obj.find(U("Temperature_Cell")) != obj.end()) {
                    try { out.mutable_batterie()->set_temperatur(obj.at(U("Temperature_Cell")).as_double()); } catch (...) {}
                }
                if (obj.find(U("Voltage")) != obj.end()) {
                    try { out.mutable_batterie()->set_spannung(obj.at(U("Voltage")).as_double()); } catch (...) {}
                }
            };
            if (Storage.is_object()) {
                // There may be multiple entries keyed by storage ID
                for (const auto &kv : Storage.as_object()) {
                    if (kv.second.is_object()) fill_from_obj(kv.second.as_object());
                }
            }
        }

        // Optionally try to read inverter AC power and frequency
        if (Data.has_field(U("Inverters"))) {
            const auto &Invs = Data.at(U("Inverters"));
            if (Invs.is_object()) {
                for (const auto &kv : Invs.as_object()) {
                    const auto &inv = kv.second;
                    if (!inv.is_object()) continue;
                    const auto &io = inv.as_object();
                    if (io.find(U("P")) != io.end()) {
                        try { out.mutable_system()->set_ac_leistung(io.at(U("P")).as_double()); } catch (...) {}
                    }
                    if (io.find(U("FREQ")) != io.end()) {
                        try { out.mutable_system()->set_frequenz(io.at(U("FREQ")).as_double()); } catch (...) {}
                    }
                }
            }
        }

        handler(out);
        return absl::OkStatus();
    });

    if (st.ok()) {
        wastlernet::metrics::WastlernetMetrics::GetInstance().fronius_query_counter.Increment();
        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end_time - start_time;
        wastlernet::metrics::WastlernetMetrics::GetInstance().fronius_duration_ms.Observe(
            std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
    } else {
        wastlernet::metrics::WastlernetMetrics::GetInstance().fronius_error_counter.Increment();
    }

    return st;
}

http_client_config FroniusClient::ClientConfig() {
    http_client_config config = HttpConnection::ClientConfig();
    config.set_validate_certificates(false);
    return config;
}

} // namespace fronius
*/