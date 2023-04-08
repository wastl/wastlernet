//
// Created by wastl on 07.04.23.
//
#include "include/module.h"
#include "solvis/solvis.pb.h"
#include "solvis/solvis_timescaledb.h"

#ifndef WASTLERNET_SOLVIS_MODULE_H
#define WASTLERNET_SOLVIS_MODULE_H
namespace solvis {
    class SolvisModule : public wastlernet::PollingModule<SolvisData> {
    private:
        std::string host;
        int port;

    protected:
        absl::Status Query(std::function<absl::Status(const SolvisData &)> handler) override;

    public:
        SolvisModule(const wastlernet::TimescaleDB &db_cfg, const wastlernet::Solvis& client_cfg)
                : wastlernet::PollingModule<SolvisData>(db_cfg, new SolvisWriter, client_cfg.poll_interval()),
                        host(client_cfg.host()), port(client_cfg.port()) { }
    };
}
#endif //WASTLERNET_SOLVIS_MODULE_H
