//
// Created by wastl on 07.04.23.
//

#include "solvis_module.h"

#include "solvis/solvis_client.h"


absl::Status solvis::SolvisModule::Query(std::function<absl::Status(const solvis::SolvisData &)> handler) {
    try {
        query(host, port, handler);
        return absl::OkStatus();
    } catch (std::exception const &e) {
        LOG(ERROR) << "Error querying Solvis controller: " << e.what();
        return absl::InternalError(e.what());
    }
}

