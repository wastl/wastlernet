//
// Created by wastl on 02.07.25.
//
#include "include/module.h"
#include "fronius/fronius.pb.h"
#include "fronius/fronius_modbus.h"
#include "fronius/fronius_timescaledb.h"

#ifndef WASTLERNET_FRONIUS_MODULE_H
#define WASTLERNET_FRONIUS_MODULE_H
namespace fronius {
    class FroniusModule : public wastlernet::PollingModule<FroniusData> {
    private:
        FroniusModbusConnection* conn_;

    protected:
        absl::Status Query(std::function<absl::Status(const FroniusData &)> handler) override;

    public:
        FroniusModule(const wastlernet::TimescaleDB &db_cfg, const wastlernet::Fronius &client_cfg,
                     FroniusModbusConnection *conn, wastlernet::StateCache *c)
                : wastlernet::PollingModule<FroniusData>(db_cfg, new FroniusWriter, c, client_cfg.poll_interval()),
                  conn_(conn) {}

        std::string Name() override {
            return "fronius";
        }
    };
}
#endif //FRONIUS_MODULE_H
