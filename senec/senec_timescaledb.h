//
// Created by wastl on 07.04.23.
//
#include <pqxx/pqxx>
#include <absl/status/status.h>
#include "timescaledb/timescaledb-client.h"
#include "senec/senec.pb.h"

#ifndef WASTLERNET_SENEC_TIMESCALEDB_H
#define WASTLERNET_SENEC_TIMESCALEDB_H
namespace senec {
    class SenecWriter : public timescaledb::TimescaleWriter<SenecData> {
    public:
        absl::Status prepare(pqxx::connection &conn) override;

        absl::Status write(pqxx::work &tx, const SenecData &data) override;
    };
}
#endif //WASTLERNET_SENEC_TIMESCALEDB_H
