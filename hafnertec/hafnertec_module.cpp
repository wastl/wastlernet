//
// Created by wastl on 07.04.23.
//

#include "hafnertec_module.h"

#include "hafnertec/hafnertec_client.h"

absl::Status hafnertec::HafnertecModule::Query(std::function<absl::Status(const hafnertec::HafnertecData &)> handler) {
    try {
        query(uri, user, password, handler);
        return absl::OkStatus();
    } catch (std::exception const &e) {
        LOG(ERROR) << "Error querying Hafnertec controller: " << e.what();
        return absl::InternalError(e.what());
    }
}
