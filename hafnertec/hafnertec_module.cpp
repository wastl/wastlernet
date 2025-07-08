//
// Created by wastl on 07.04.23.
//

#include "hafnertec_module.h"

#include "hafnertec/hafnertec_client.h"

absl::Status hafnertec::HafnertecModule::Query(std::function<absl::Status(const hafnertec::HafnertecData &)> handler) {
    try {
        return client_.Query(handler);
    } catch (std::exception const &e) {
        LOG(ERROR) << "Error querying Hafnertec controller: " << e.what();
        return absl::InternalError(e.what());
    }
}

absl::Status hafnertec::HafnertecModule::Init() {
    auto st = PollingModule<HafnertecData>::Init();
    if (!st.ok()) {
        return st;
    }
    return client_.Init();
}
