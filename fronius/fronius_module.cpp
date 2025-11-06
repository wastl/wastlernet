//
// Created by wastl on 02.07.25.
//

#include "fronius_module.h"
#include "base/utility.h"

#include <glog/logging.h>
#include <chrono>

#define LOGF(level) LOG(level) << "[fronius] "

namespace fronius {

absl::Status FroniusModule::Query(std::function<absl::Status(const fronius::FroniusData &)> handler) {
    try {
        FroniusData data;

        RETURN_IF_ERROR(pf_client_.Query([&](const Leistung &l, const Quellen &q) {
            *data.mutable_leistung() = l;
        }));
        RETURN_IF_ERROR(slave_client_.Query([&](const Leistung &l, const Quellen &q) {
            data.mutable_leistung()->set_pv_leistung(data.leistung().pv_leistung() + l.pv_leistung());
        }));
        RETURN_IF_ERROR(energy_client_.Query([&](double consumption) {
            data.mutable_leistung()->set_hausverbrauch(consumption);
        }));
        RETURN_IF_ERROR(battery_client_.Query([&](const Batterie &b) {
            *data.mutable_batterie() = b;
        }));

        // Fix up consumption
        double consumption = data.leistung().pv_leistung()+data.leistung().batterie_leistung()+data.leistung().netz_leistung();
        data.mutable_leistung()->set_hausverbrauch(consumption);

        // Fix up sources
        Quellen quellen;
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


        return handler(data);
    } catch (std::exception const &e) {
        LOGF(ERROR) << "Error querying Fronius Solar API: " << e.what();
        return absl::InternalError(e.what());
    }
}

absl::Status FroniusModule::Init() {
    RETURN_IF_ERROR(PollingModule<FroniusData>::Init());
    RETURN_IF_ERROR(pf_client_.Init());
    RETURN_IF_ERROR(battery_client_.Init());
    return absl::OkStatus();
}

} // namespace fronius