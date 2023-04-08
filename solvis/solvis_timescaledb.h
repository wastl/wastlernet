//
// Created by wastl on 05.04.23.
//

#include <pqxx/pqxx>
#include <absl/status/status.h>
#include "timescaledb/timescaledb-client.h"
#include "solvis/solvis.pb.h"

#ifndef WASTLERNET_SOLVIS_TIMESCALEDB_H
#define WASTLERNET_SOLVIS_TIMESCALEDB_H
namespace solvis {
    class SolvisWriter : public timescaledb::TimescaleWriter<SolvisData> {
    public:
        absl::Status prepare(pqxx::connection &conn) override;

        absl::Status write(pqxx::work &tx, const SolvisData &data) override;
    };
}
#endif //WASTLERNET_SOLVIS_TIMESCALEDB_H
