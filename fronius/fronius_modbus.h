//
// Created by wastl on 02.07.25.
//

#include <string>

#include "base/modbus_connection.h"
#include "config/config.pb.h"

#ifndef WASTLERNET_FRONIUS_MODBUS_H
#define WASTLERNET_FRONIUS_MODBUS_H
namespace fronius {
    class FroniusModbusConnection : public wastlernet::ModbusConnection {
    public:
        explicit FroniusModbusConnection(const wastlernet::Fronius& client_cfg)
        : wastlernet::ModbusConnection(client_cfg.host(), client_cfg.port(), 40001, 2) { }

    protected:
        std::string Name() override { return "FroniusModbusConnection"; }
    };

}

#endif //FRONIUS_MODBUS_H
