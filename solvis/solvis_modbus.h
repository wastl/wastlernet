//
// Created by wastl on 30.10.23.
//
#include <string>


#include "base/modbus_connection.h"
#include "config/config.pb.h"

#ifndef WASTLERNET_SOLVIS_MODBUS_H
#define WASTLERNET_SOLVIS_MODBUS_H
namespace solvis {
    class SolvisModbusConnection : public wastlernet::ModbusConnection {
    public:
        explicit SolvisModbusConnection(const wastlernet::Solvis& client_cfg)
        : ModbusConnection(client_cfg.host(), client_cfg.port(), 32770) { }

    protected:
        std::string Name() override {
            return "SolvisModbusConnection";
        }
    };

}
#endif //WASTLERNET_SOLVIS_MODBUS_H
