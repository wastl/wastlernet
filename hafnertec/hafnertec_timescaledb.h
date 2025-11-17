//
// Created by wastl on 07.04.23.
//
#pragma once
#include <pqxx/pqxx>
#include <absl/status/status.h>
#include "timescaledb/timescaledb-client.h"
#include "hafnertec/hafnertec.pb.h"

#ifndef WASTLERNET_HAFNERTEC_TIMESCALEDB_H
#define WASTLERNET_HAFNERTEC_TIMESCALEDB_H
namespace hafnertec {
    class HafnertecWriter : public timescaledb::TimescaleWriter<HafnertecData> {
    public:
        absl::Status prepare(pqxx::connection &conn) override;

        absl::Status write(pqxx::work &tx, const HafnertecData &data) override;
    };
}
#endif //WASTLERNET_HAFNERTEC_TIMESCALEDB_H
