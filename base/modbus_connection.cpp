//
// Created by wastl on 03.07.25.
//
#include "modbus_connection.h"
#include "base/utility.h"

#include <absl/strings/str_cat.h>
#include <glog/logging.h>
#include <modbus/modbus.h>
#include <modbus/modbus-tcp.h>

#define LOGS(level) LOG(level) << "[" << Name() << "] "


wastlernet::ModbusConnection::~ModbusConnection() {
    modbus_close(ctx_);
    modbus_free(ctx_);
}

absl::Status wastlernet::ModbusConnection::Init() {
    absl::MutexLock lock(&mutex_);

    LOGS(INFO) << "Initializing Modbus connection";

    ctx_ = modbus_new_tcp(host_.c_str(), port_);
    if (ctx_ == nullptr) {
        LOGS(ERROR) << "Unable to allocate libmodbus context";
        return absl::InternalError("Unable to allocate libmodbus context");
    }

    auto st = wastlernet::retry_with_backoff([this]() {
        int rc = modbus_connect(ctx_);
        if (rc == -1) {
            return absl::InternalError(absl::StrCat("Connection failed: ", modbus_strerror(errno)));
        }
        return absl::OkStatus();
    }, 3);

    if (st.ok()) {
        initialized_ = true;
        LOGS(INFO) << "Modbus connection to " << host_ << ":" << port_ << " established successfully";
    } else {
        LOGS(ERROR) << st;
    }

    return st;
}

absl::Status wastlernet::ModbusConnection::Execute(const std::function<absl::Status(modbus_t *)>& method) {
    if (!initialized_) {
        return absl::FailedPreconditionError("Modbus connection not initialized");
    }


    auto st = Reinit();

    absl::MutexLock lock(&mutex_);
    if (!st.ok()) {
        return st;
    }

    return method(ctx_);
}

absl::Status wastlernet::ModbusConnection::Reinit() {
    // Check if a Reinit is necessary by reading e.g. the Solvis version
    uint16_t reg[init_count_];
    if (init_addr_ >= 0) {
        if (int rc = modbus_read_registers(ctx_, init_addr_, init_count_, reg); rc == -1) {
            LOGS(INFO) << "Re-initializing Modbus connection";

            modbus_close(ctx_);
            modbus_free(ctx_);

            return Init();
        }
    }
    return absl::OkStatus();
}


int16_t wastlernet::ModbusConnection::toInt16(const uint16_t *u) {
    // Use a union for type punning to re-interpret the 16-bit unsigned integer as a 16-bit signed integer.
    union {
        uint16_t u16;
        int16_t i16;
    } converter {};
    converter.u16 = *u;
    return converter.i16;
}

int32_t wastlernet::ModbusConnection::toInt32(const uint16_t *u) {
    // Use a union for type punning to re-interpret the 32-bit unsigned integer as a 32-bit signed integer.
   union {
        uint32_t u32;
        int32_t i32;
    } converter {};

    converter.u32 = ((uint32_t)u[0] << 16) | u[1];
    return converter.i32;
}

int64_t wastlernet::ModbusConnection::toInt64(const uint16_t *u) {
    return ((uint64_t)u[0] << 48) | ((uint64_t)u[1] << 32) | ((uint64_t)u[2] << 16) | u[3];
}

float wastlernet::ModbusConnection::toFloat(const uint16_t *u) {
    // Use a union for type punning to re-interpret the 32-bit unsigned integer
    // as a 32-bit floating-point number (IEEE 754 single-precision).
    union {
        uint32_t u32;
        float f32;
    } converter {};

    converter.u32 = ((uint32_t)u[0] << 16) | u[1];

    // Return the float representation
    return converter.f32;
}

std::string wastlernet::ModbusConnection::toString(const uint16_t *u, int nchars) {
    return { u, u+nchars };
}

std::bitset<16> wastlernet::ModbusConnection::toBitset16(const uint16_t *u) {
    return { *u };
}

std::bitset<32> wastlernet::ModbusConnection::toBitset32(const uint16_t *u) {
    return { static_cast<unsigned long long>((u[0] << 16) + u[1]) };
}
