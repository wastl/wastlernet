//
// Created by wastl on 08.04.23.
//
#include "include/module.h"
#include "senec/senec.pb.h"
#include "senec/senec_timescaledb.h"

#ifndef WASTLERNET_SENEC_MODULE_H
#define WASTLERNET_SENEC_MODULE_H
namespace senec {
    class SenecModule : public wastlernet::PollingModule<SenecData> {
    private:
        std::string uri;

    protected:
        absl::Status Query(std::function<absl::Status(const SenecData &)> handler) override;

    public:
        SenecModule(const wastlernet::TimescaleDB& db_cfg, const wastlernet::Senec& client_cfg, wastlernet::StateCache* c)
                : wastlernet::PollingModule<SenecData>(db_cfg, new SenecWriter, c, client_cfg.poll_interval()),
                  uri(client_cfg.host()) {}

        std::string Name() override {
            return "senec";
        }
    };
};
#endif //WASTLERNET_SENEC_MODULE_H
