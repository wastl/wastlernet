//
// Created by wastl on 08.04.23.
//

#include "senec_module.h"
#include "senec_client.h"

absl::Status senec::SenecModule::Query(std::function<absl::Status(const SenecData &)> handler) {
    try {
        senec::query(uri, handler);
        return absl::OkStatus();
    } catch (std::exception const &e) {
        LOG(ERROR) << "Error querying SENEC controller: " << e.what();
        return absl::InternalError(e.what());
    }
}
