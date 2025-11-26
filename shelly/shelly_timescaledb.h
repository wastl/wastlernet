//
// Created by wastl on 26.11.25.
//

#include <pqxx/pqxx>
#include <absl/status/status.h>
#include "timescaledb/timescaledb-client.h"
#include "shelly/shelly.pb.h"

#ifndef WASTLERNET_SHELLY_TIMESCALEDB_H
#define WASTLERNET_SHELLY_TIMESCALEDB_H

namespace wastlernet::shelly {

    class ShellyWriter : public timescaledb::TimescaleWriter<ShellyData> {
    public:
        absl::Status prepare(pqxx::connection &conn) override;

        absl::Status write(pqxx::work &tx, const ShellyData &data) override;
    };

}

#endif // WASTLERNET_SHELLY_TIMESCALEDB_H
