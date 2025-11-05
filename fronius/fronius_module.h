//
// Created by wastl on 02.07.25.
//
#include "base/module.h"
#include "fronius/fronius.pb.h"
#include "fronius/fronius_client.h"
#include "fronius/fronius_timescaledb.h"

#ifndef WASTLERNET_FRONIUS_MODULE_H
#define WASTLERNET_FRONIUS_MODULE_H
namespace fronius {
    class FroniusModule : public wastlernet::PollingModule<FroniusData> {
    private:
        FroniusPowerFlowClient pf_client_;
        FroniusBatteryClient battery_client_;

    protected:
        absl::Status Query(std::function<absl::Status(const FroniusData &)> handler) override;

    public:
        FroniusModule(const wastlernet::TimescaleDB &db_cfg, const wastlernet::Fronius &client_cfg,
                      wastlernet::StateCache *c)
                : wastlernet::PollingModule<FroniusData>(db_cfg, new FroniusWriter, c, client_cfg.poll_interval()),
                  pf_client_(client_cfg.host().rfind("http", 0) == 0 ? client_cfg.host() : std::string("http://") + client_cfg.host()),
                  battery_client_(client_cfg.host().rfind("http", 0) == 0 ? client_cfg.host() : std::string("http://") + client_cfg.host())
        {}

        std::string Name() override {
            return "fronius";
        }

        absl::Status Init() override;
    };
}
#endif //FRONIUS_MODULE_H
