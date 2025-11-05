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
            *data.mutable_quellen() = q;
        }));
        RETURN_IF_ERROR(battery_client_.Query([&](const Batterie &b) {
            *data.mutable_batterie() = b;
        }));


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