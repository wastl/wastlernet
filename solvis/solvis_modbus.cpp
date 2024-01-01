//
// Created by wastl on 30.10.23.
//

#include "solvis_modbus.h"
#include "include/utility.h"

#include <absl/strings/str_cat.h>
#include <glog/logging.h>
#include <modbus/modbus.h>
#include <modbus/modbus-tcp.h>

#define LOGS(level) LOG(level) << "[solvis] "



solvis::SolvisModbusConnection::~SolvisModbusConnection() {
    modbus_close(ctx_);
    modbus_free(ctx_);
}

absl::Status solvis::SolvisModbusConnection::Init() {
    LOGS(INFO) << "Initializing Modbus connection";

    ctx_ = modbus_new_tcp(host_.c_str(), port_);
    if (ctx_ == nullptr) {
        LOGS(ERROR) << "Unable to allocate libmodbus context";
        return absl::InternalError("Unable to allocate libmodbus context");
    }

    auto st = wastlernet::retry_with_backoff([this]() {
        int rc = modbus_connect(ctx_);
        if (rc == -1) {
            return absl::InternalError(absl::StrCat("Connection failed: ",modbus_strerror(errno)));
        }
        return absl::OkStatus();
    }, 3);

    if (st.ok()) {
        LOGS(INFO) << "Modbus connection to " << host_ << ":" << port_ << " established successfully";
    } else {
        LOGS(ERROR) << st;
    }

    return st;
}

absl::Status solvis::SolvisModbusConnection::Execute(std::function<absl::Status(modbus_t *)> method) {
    absl::MutexLock lock(&mutex_);

    auto st = Reinit();
    if (!st.ok()) {
        return st;
    }

    return method(ctx_);
}

absl::Status solvis::SolvisModbusConnection::Reinit() {
    // Check if a Reinit is necessary by reading e.g. the Solvis version
    uint16_t reg[2];
    int rc = modbus_read_registers(ctx_, 32770, 2, reg);
    if (rc == -1) {
        LOGS(INFO) << "Re-initializing Modbus connection";

        modbus_close(ctx_);
        modbus_free(ctx_);

        return Init();
    }
    return absl::OkStatus();
}
