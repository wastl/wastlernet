//
// Created by wastl on 02.07.25.
//
#include <pqxx/pqxx>
#include <absl/status/status.h>
#include "timescaledb/timescaledb-client.h"
#include "fronius/fronius.pb.h"

#ifndef WASTLERNET_FRONIUS_TIMESCALEDB_H
#define WASTLERNET_FRONIUS_TIMESCALEDB_H
namespace fronius {
    class FroniusWriter : public timescaledb::TimescaleWriter<FroniusData> {
    public:
        absl::Status prepare(pqxx::connection &conn) override;

        absl::Status write(pqxx::work &tx, const FroniusData &data) override;
    };
}
#endif //WASTLERNET_FRONIUS_TIMESCALEDB_H
