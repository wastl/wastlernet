//
// Created by wastl on 30.10.23.
//
#include <string>
#include <absl/status/status.h>
#include <absl/synchronization/mutex.h>
#include <modbus/modbus.h>
#include <modbus/modbus-tcp.h>

#include "config/config.pb.h"

#ifndef WASTLERNET_SOLVIS_MODBUS_H
#define WASTLERNET_SOLVIS_MODBUS_H
namespace solvis {
    class SolvisModbusConnection {
    public:
        SolvisModbusConnection(const wastlernet::Solvis& client_cfg)
        : host_(client_cfg.host()), port_(client_cfg.port()) { }

        ~SolvisModbusConnection();

        // Initialize Modbus Context. Must be called before using Execute.
        absl::Status Init();

        // Execute the function passed as argument using the (established) modbus context managed by this class.
        absl::Status Execute(std::function<absl::Status(modbus_t*)> method);

    private:
        absl::Mutex mutex_;

        std::string host_;
        int port_;

        modbus_t *ctx_;

        // Check if a reinit is necessary and re-establish the connection if needed.
        absl::Status Reinit();
    };

}
#endif //WASTLERNET_SOLVIS_MODBUS_H
