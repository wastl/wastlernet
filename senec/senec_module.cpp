//
// Created by wastl on 08.04.23.
//

#include "senec_module.h"
#include "senec_client.h"

absl::Status senec::SenecModule::Query(std::function<absl::Status(const SenecData &)> handler) {
    try {
        return client_.Query(handler);
    } catch (std::exception const &e) {
        LOG(ERROR) << "Error querying SENEC controller: " << e.what();
        return absl::InternalError(e.what());
    }
}

absl::Status senec::SenecModule::Init() {
    auto st = PollingModule<SenecData>::Init();
    if (!st.ok()) {
        return st;
    }
    return client_.Init();
}
