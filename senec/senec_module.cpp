//
// Created by wastl on 08.04.23.
//

#include "senec_module.h"
#include "senec_client.h"

absl::Status senec::SenecModule::Query(std::function<absl::Status(const SenecData &)> handler) {
    try {
        return senec::query(uri, handler);
    } catch (std::exception const &e) {
        LOG(ERROR) << "Error querying SENEC controller: " << e.what();
        return absl::InternalError(e.what());
    }
}
