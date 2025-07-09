//
// Created by wastl on 18.01.22.
//

#include <iostream>
#include <cpprest/uri.h>                        // URI library
#include <cpprest/http_msg.h>
#include <cpprest/http_client.h>
#include <cpprest/json.h>
#include <glog/logging.h>
#include <absl/strings/str_cat.h>

#include "base/metrics.h"
#include "senec_client.h"

using namespace utility; // Common utilities like string conversions
using namespace web; // Common features like URIs.
using namespace web::http; // Common HTTP functionality
using namespace web::http::client; // HTTP client features
using namespace concurrency::streams; // Asynchronous streams
using namespace web::json; // JSON library

#define LOGS(level) LOG(level) << "[senec] "

namespace {
    float hex2float(const std::string &s) {
        uint32_t num;
        sscanf(s.c_str(), "%x", &num); // assuming you checked input
        return *((float *) &num);
    }

    template<typename T>
    T senec_parse(const json::value &v) {
        if (!v.is_string()) {
            LOGS(ERROR) << "Warning: value is not a string: " << v.serialize();
            return 0;
        }

        std::string s = v.as_string();

        if (s.rfind("u1", 0) == 0 || s.rfind("u3", 0) == 0 || s.rfind("u8", 0) == 0) {
            return std::stoul(s.substr(3), nullptr, 16);
        }
        if (s.rfind("i1", 0) == 0) {
            return std::stol(s.substr(3), nullptr, 16);
        }
        if (s.rfind("fl", 0) == 0) {
            return hex2float(s.substr(3));
        }
        if (s.rfind("st", 0) == 0) {
            LOGS(ERROR) << "Warning: value is not a number: " << v.serialize();
            return 0;
        }
        return 0;
    }
}

senec::SenecClient::SenecClient(const std::string &base_url)
    : HttpConnection(base_url, "/lala.cgi", POST) {
    request_body_["PV1"]["POWER_RATIO"] = json::value::string("");
    request_body_["PM1OBJ1"]["P_TOTAL"] = json::value::string("");
    request_body_["PM1OBJ1"]["FREQ"] = json::value::string("");
    request_body_["PM1OBJ1"]["U_AC"] = json::value::string("");
    request_body_["PM1OBJ1"]["I_AC"] = json::value::string("");
    request_body_["PM1OBJ1"]["P_AC"] = json::value::string("");
    request_body_["ENERGY"]["GUI_BAT_DATA_FUEL_CHARGE"] = json::value::string("");
    request_body_["ENERGY"]["GUI_BAT_DATA_POWER"] = json::value::string("");
    request_body_["ENERGY"]["GUI_BAT_DATA_VOLTAGE"] = json::value::string("");
    request_body_["ENERGY"]["GUI_HOUSE_POW"] = json::value::string("");
    request_body_["ENERGY"]["GUI_GRID_POW"] = json::value::string("");
    request_body_["ENERGY"]["GUI_INVERTER_POWER"] = json::value::string("");
    request_body_["ENERGY"]["STAT_STATE"] = json::value::string("");
    request_body_["ENERGY"]["STAT_HOURS_OF_OPERATION"] = json::value::string("");
    request_body_["BMS"]["NR_INSTALLED"] = json::value::string("");
    request_body_["BMS"]["TOTAL_CURRENT"] = json::value::string("");
    request_body_["STATISTIC"]["LIVE_GRID_IMPORT"] = json::value::string("");
    request_body_["STATISTIC"]["LIVE_GRID_EXPORT"] = json::value::string("");
    request_body_["STATISTIC"]["LIVE_HOUSE_CONS"] = json::value::string("");
    request_body_["STATISTIC"]["LIVE_PV_GEN"] = json::value::string("");
    request_body_["STATISTIC"]["CURRENT_STATE"] = json::value::string("");
    request_body_["TEMPMEASURE"]["BATTERY_TEMP"] = json::value::string("");
    request_body_["TEMPMEASURE"]["CASE_TEMP"] = json::value::string("");
    request_body_["TEMPMEASURE"]["MCU_TEMP"] = json::value::string("");
    request_body_["FAN_SPEED"]["INV_LV"] = json::value::string("");
    request_body_["PV1"]["MPP_CUR"] = json::value::string("");
    request_body_["PV1"]["MPP_VOL"] = json::value::string("");
    request_body_["PV1"]["MPP_POWER"] = json::value::string("");
}

absl::Status senec::SenecClient::Query(const std::function<void(const SenecData &)> &handler) {
    auto start_time = std::chrono::high_resolution_clock::now();

    auto st = Execute([=](const http_response &response) {
        if (response.status_code() == status_codes::OK) {
            json::value result = response.extract_json(true).get();

            LOGS(INFO) << "Received data from Senec controller";

            senec::SenecData data;
            data.mutable_system()->set_pv_begrenzung(senec_parse<int>(result["PV1"]["POWER_RATIO"]));
            data.mutable_system()->set_ac_leistung(senec_parse<double>(result["PM1OBJ1"]["P_TOTAL"]));
            data.mutable_system()->set_frequenz(senec_parse<double>(result["PM1OBJ1"]["FREQ"]));
            data.mutable_batterie()->set_soc(senec_parse<double>(result["ENERGY"]["GUI_BAT_DATA_FUEL_CHARGE"]));
            data.mutable_batterie()->set_leistung(senec_parse<double>(result["ENERGY"]["GUI_BAT_DATA_POWER"]));
            data.mutable_batterie()->set_spannung(senec_parse<double>(result["ENERGY"]["GUI_BAT_DATA_VOLTAGE"]));
            data.mutable_batterie()->set_temperatur(senec_parse<double>(result["TEMPMEASURE"]["BATTERY_TEMP"]));
            data.mutable_leistung()->set_hausverbrauch(senec_parse<double>(result["ENERGY"]["GUI_HOUSE_POW"]));
            data.mutable_leistung()->set_netz_leistung(senec_parse<double>(result["ENERGY"]["GUI_GRID_POW"]));
            data.mutable_leistung()->set_pv_leistung(senec_parse<double>(result["ENERGY"]["GUI_INVERTER_POWER"]));
            data.mutable_leistung()->set_batterie_leistung(senec_parse<double>(result["ENERGY"]["GUI_BAT_DATA_POWER"]));
            data.mutable_system()->set_status(senec_parse<int>(result["ENERGY"]["STAT_STATE"]));
            data.mutable_system()->set_betriebsstunden(senec_parse<int>(result["ENERGY"]["STAT_HOURS_OF_OPERATION"]));
            data.mutable_system()->set_anzahl_batterien(senec_parse<int>(result["BMS"]["NR_INSTALLED"]));
            data.mutable_gesamt()->set_bezug(senec_parse<double>(result["STATISTIC"]["LIVE_GRID_IMPORT"]) * 1000);
            data.mutable_gesamt()->set_strom(senec_parse<double>(result["BMS"]["TOTAL_CURRENT"]));
            data.mutable_gesamt()->set_einspeisung(senec_parse<double>(result["STATISTIC"]["LIVE_GRID_EXPORT"]) * 1000);
            data.mutable_gesamt()->set_verbrauch(senec_parse<double>(result["STATISTIC"]["LIVE_HOUSE_CONS"]) * 1000);
            data.mutable_gesamt()->set_produktion(senec_parse<double>(result["STATISTIC"]["LIVE_PV_GEN"]) * 1000);
            data.mutable_system()->set_gehaeuse_temperatur(senec_parse<double>(result["TEMPMEASURE"]["CASE_TEMP"]));
            data.mutable_system()->set_mcu_temperatur(senec_parse<double>(result["TEMPMEASURE"]["MCU_TEMP"]));
            data.mutable_system()->set_fan_speed(senec_parse<int>(result["FAN_SPEED"]["INV_LV"]));

            for (int i = 0; i < 3; i++) {
                auto ac_data = data.add_ac_data();
                ac_data->set_spannung(senec_parse<double>(result["PM1OBJ1"]["U_AC"][i]));
                ac_data->set_strom(senec_parse<double>(result["PM1OBJ1"]["I_AC"][i]));
                ac_data->set_leistung(senec_parse<double>(result["PM1OBJ1"]["P_AC"][i]));
            }

            for (int i = 0; i < 3; i++) {
                auto mppt = data.add_mppt();
                mppt->set_strom(senec_parse<double>(result["PV1"]["MPP_CUR"][i]));
                mppt->set_spannung(senec_parse<double>(result["PV1"]["MPP_VOL"][i]));
                mppt->set_leistung(senec_parse<double>(result["PV1"]["MPP_POWER"][i]));
            }

            if (data.batterie().leistung() < 0) {
                data.mutable_quellen()->set_laden(0);
                data.mutable_quellen()->set_entladen(-data.batterie().leistung());
            } else {
                data.mutable_quellen()->set_laden(data.batterie().leistung());
                data.mutable_quellen()->set_entladen(0);
            }

            if (data.leistung().netz_leistung() < 0) {
                data.mutable_quellen()->set_einspeisung(-data.leistung().netz_leistung());
                data.mutable_quellen()->set_bezug(0);
            } else {
                data.mutable_quellen()->set_einspeisung(0);
                data.mutable_quellen()->set_bezug(data.leistung().netz_leistung());
            }

            LOGS(INFO) << "running handler";

            handler(data);


            return absl::OkStatus();
        } else {
            LOGS(ERROR) << "SENEC query failed: " << response.reason_phrase();


            return absl::InternalError(absl::StrCat("SENEC query failed: ", response.reason_phrase()));
        }
    });

    if (st.ok()) {
        wastlernet::metrics::WastlernetMetrics::GetInstance().senec_query_counter.Increment();

        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end_time - start_time;
        wastlernet::metrics::WastlernetMetrics::GetInstance().senec_duration_ms.Observe(
            std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
    } else {
        wastlernet::metrics::WastlernetMetrics::GetInstance().senec_error_counter.Increment();
    }


    return st;
}

web::http::client::http_client_config senec::SenecClient::ClientConfig() {
    http_client_config config = HttpConnection::ClientConfig();
    config.set_validate_certificates(false);
    return config;
}

std::optional<web::json::value> senec::SenecClient::RequestBody() {
    return request_body_;
}
