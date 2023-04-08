//
// Created by wastl on 07.04.23.
//
#include "include/module.h"
#include "hafnertec/hafnertec.pb.h"
#include "hafnertec/hafnertec_timescaledb.h"

#ifndef WASTLERNET_HAFNERTEC_MODULE_H
#define WASTLERNET_HAFNERTEC_MODULE_H
namespace hafnertec {
    class HafnertecModule : public wastlernet::PollingModule<HafnertecData> {
    private:
        std::string uri;
        std::string user;
        std::string password;

    protected:
        absl::Status Query(std::function<absl::Status(const HafnertecData &)> handler) override;

    public:
        HafnertecModule(const wastlernet::TimescaleDB& db_cfg, const wastlernet::Hafnertec& client_cfg)
        : wastlernet::PollingModule<HafnertecData>(db_cfg, new HafnertecWriter, client_cfg.poll_interval()),
                uri(client_cfg.host()), user(client_cfg.user()), password(client_cfg.password()) {}
    };
};
#endif //WASTLERNET_HAFNERTEC_MODULE_H
