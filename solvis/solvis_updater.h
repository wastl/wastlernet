//
// Created by wastl on 27.10.23.
//
#include "include/updater.h"
#include "solvis/solvis.pb.h"
#include "solvis/solvis_modbus.h"

#ifndef WASTLERNET_SOLVIS_UPDATER_H
#define WASTLERNET_SOLVIS_UPDATER_H
namespace solvis {
    class SolvisUpdater : public wastlernet::PollingUpdater<SolvisData> {
    private:
        SolvisModbusConnection* conn_;

    protected:
        absl::Status Update() override;

    public:
        SolvisUpdater(const wastlernet::Solvis& client_cfg, SolvisModbusConnection* conn, wastlernet::StateCache* c)
                : wastlernet::PollingUpdater<SolvisData>(c, client_cfg.poll_interval()),
                  conn_(conn) { }

    };
}

#endif //WASTLERNET_SOLVIS_UPDATER_H
