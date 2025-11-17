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
#include <cmath>
#include <algorithm>

#include "base/metrics.h"
#include "fronius_client.h"


#define LOGF(level) LOG(level) << "[fronius] "

namespace fronius {
    using namespace web; // Common features like URIs.
    using namespace web::http; // Common HTTP functionality
    using namespace web::http::client; // HTTP client features
    using namespace web::json; // JSON library

    // Small helpers to traverse cpprestsdk JSON more succinctly.
    namespace {
        inline utility::string_t s2t(const char* s) {
            return utility::conversions::to_string_t(s);
        }

        // Walk a nested object path like {"Body","Data","Site"}. Returns nullptr if missing.
        inline const value* json_getp(const value& root, std::initializer_list<const char*> path) {
            const value* cur = &root;
            for (const char* k : path) {
                if (!cur->is_object()) return nullptr;
                const auto key = s2t(k);
                const auto& obj = cur->as_object();
                auto it = obj.find(key);
                if (it == obj.end()) return nullptr;
                cur = &it->second;
            }
            return cur;
        }

        // Get a double field from an object; returns def if not present or not a number.
        inline double json_get_double(const value& obj, const char* key, double def = 0.0) {
            try {
                if (!obj.is_object()) return def;
                auto sk = s2t(key);
                const auto& o = obj.as_object();
                auto it = o.find(sk);
                if (it == o.end()) return def;
                const auto& v = it->second;
                if (v.is_number()) return v.as_double();
                // Some Fronius firmwares have numeric-ish strings; try conversion.
                if (v.is_string()) {
                    try {
                        return std::stod(utility::conversions::to_utf8string(v.as_string()));
                    } catch (...) {
                        return def;
                    }
                }
            } catch (...) {
            }
            return def;
        }
    } // namespace

    absl::Status FroniusPowerFlowClient::Query(const std::function<void(const Leistung&, const Quellen&)>& handler) {
        auto start_time = std::chrono::high_resolution_clock::now();

        auto st = Execute([&](const http_response& response) {
            if (response.status_code() != status_codes::OK) {
                LOGF(ERROR) << "Fronius query failed: " << response.reason_phrase();
                return absl::InternalError(absl::StrCat("Fronius query failed: ", response.reason_phrase()));
            }

            value result = response.extract_json(true).get();

            // Drill down to Body/Data/Site in a compact way.
            const value* Site = json_getp(result, {"Body","Data","Site"});
            if (!Site) {
                LOGF(ERROR) << "Unexpected JSON: missing Body.Data.Site: " << result.serialize();
                return absl::InternalError("Unexpected Fronius JSON (no Body.Data.Site)");
            }

            Leistung data;

            // Extract doubles with sensible defaults (0.0 if missing)
            double p_load = json_get_double(*Site, "P_Load"); // W, positive consumption
            double p_pv = json_get_double(*Site, "P_PV"); // W, PV production
            double p_grid = json_get_double(*Site, "P_Grid"); // W, positive import, negative export
            double p_akku = json_get_double(*Site, "P_Akku"); // W, positive discharge, negative charge

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

        {
            auto end_time = std::chrono::high_resolution_clock::now();
            const double seconds = std::chrono::duration<double>(end_time - start_time).count();
            wastlernet::metrics::WastlernetMetrics::GetInstance().ObserveQueryLatency("fronius", seconds);
            wastlernet::metrics::WastlernetMetrics::GetInstance().RecordQueryResult("fronius", st.ok());
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

            // Body/Data/0/Controller
            const value* Controller = json_getp(result, {"Body","Data","0","Controller"});
            if (!Controller) {
                LOGF(ERROR) << "Unexpected JSON: missing Body.Data.0.Controller: " << result.serialize();
                return absl::InternalError("Unexpected Fronius JSON (no Body.Data.0.Controller)");
            }

            Batterie data;

            double p_soc = json_get_double(*Controller, "StateOfCharge_Relative"); // %
            double p_current_dc = json_get_double(*Controller, "Current_DC"); // A
            double p_temp = json_get_double(*Controller, "Temperature_Cell"); // °C
            double p_voltage_dc = json_get_double(*Controller, "Voltage_DC"); // V

            data.set_soc(p_soc);
            data.set_spannung(p_voltage_dc);
            data.set_temperatur(p_temp);
            data.set_leistung(p_current_dc);

            handler(data);
            return absl::OkStatus();
        });

        {
            auto end_time = std::chrono::high_resolution_clock::now();
            const double seconds = std::chrono::duration<double>(end_time - start_time).count();
            wastlernet::metrics::WastlernetMetrics::GetInstance().ObserveQueryLatency("fronius", seconds);
            wastlernet::metrics::WastlernetMetrics::GetInstance().RecordQueryResult("fronius", st.ok());
        }

        return st;
    }


    absl::Status FroniusEnergyMeterClient::Query(const std::function<void(double consumption_watts)> &handler) {
        auto start_time = std::chrono::high_resolution_clock::now();

        auto st = Execute([&](const http_response& response) {
            if (response.status_code() != status_codes::OK) {
                LOGF(ERROR) << "Fronius query failed: " << response.reason_phrase();
                return absl::InternalError(absl::StrCat("Fronius query failed: ", response.reason_phrase()));
            }

            value result = response.extract_json(true).get();

            // Body/Data/0
            const value* Meter0 = json_getp(result, {"Body","Data","0"});
            if (!Meter0) {
                LOGF(ERROR) << "Unexpected JSON: missing Body.Data.0: " << result.serialize();
                return absl::InternalError("Unexpected Fronius JSON (no Body.Data.0)");
            }

            // Prefer per-phase powers if available
            double p1 = json_get_double(*Meter0, "PowerApparent_S_Phase_1", NAN);
            double p2 = json_get_double(*Meter0, "PowerApparent_S_Phase_2", NAN);
            double p3 = json_get_double(*Meter0, "PowerApparent_S_Phase_3", NAN);

            double consumption = 0.0;
            if (!std::isnan(p1) && !std::isnan(p2) && !std::isnan(p3)) {
                // Total house consumption as sum of positive per-phase power (import on that phase)
                auto pos = [](double v){ return v > 0 ? v : 0.0; };
                consumption = pos(p1) + pos(p2) + pos(p3);
            } else {
                double psum = json_get_double(*Meter0, "PowerApparent_S_Sum", 0.0);
                consumption = std::max(0.0, psum);
            }

            handler(consumption);
            return absl::OkStatus();
        });

        {
            auto end_time = std::chrono::high_resolution_clock::now();
            const double seconds = std::chrono::duration<double>(end_time - start_time).count();
            wastlernet::metrics::WastlernetMetrics::GetInstance().ObserveQueryLatency("fronius", seconds);
            wastlernet::metrics::WastlernetMetrics::GetInstance().RecordQueryResult("fronius", st.ok());
        }

        return st;
    }

    http_client_config FroniusBaseClient::ClientConfig() {
        http_client_config config = HttpConnection::ClientConfig();
        config.set_validate_certificates(false);
        return config;
    }

}