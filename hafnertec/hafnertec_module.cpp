//
// Created by wastl on 07.04.23.
//

#include "hafnertec_module.h"

#include "hafnertec/hafnertec_client.h"

absl::Status hafnertec::HafnertecModule::Query(std::function<absl::Status(const hafnertec::HafnertecData &)> handler) {
    try {
        return query(uri, user, password, handler);
    } catch (std::exception const &e) {
        LOG(ERROR) << "Error querying Hafnertec controller: " << e.what();
        return absl::InternalError(e.what());
    }
}
