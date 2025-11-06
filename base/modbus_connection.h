//
// Created by wastl on 03.07.25.
//
// Modbus connection helper.
//
// This header defines the wastlernet::ModbusConnection base class which encapsulates
// the boilerplate around establishing and maintaining a libmodbus TCP connection and
// provides convenience utilities to convert raw Modbus register values to native types.
//
// Responsibilities
// - Lazily initialize and maintain a modbus_t context (TCP) to a given host:port.
// - Serialize access and automatically re-initialize the context on failures.
// - Provide Execute() to run user code against the active context.
// - Provide conversion helpers for typical Modbus register layouts.
//
// Thread-safety
// All public methods are internally synchronized via an absl::Mutex. Multiple threads may
// call Init() and Execute() concurrently on the same instance. The callback passed to Execute()
// is executed while holding the connection; keep it short and avoid blocking operations.
//
// Usage
// - Construct with host, port and optional initial register range (for a light-weight connectivity check).
// - Call Init() before the first Execute(); subsequent calls are cheap no-ops.
// - Implement Name() in derived classes to aid logging/diagnostics.
// - Use the static conversion helpers to interpret register buffers returned by libmodbus.
//
#include <bitset>
#include <string>
#include <absl/status/status.h>
#include <absl/synchronization/mutex.h>
#include <modbus/modbus.h>
#include <modbus/modbus-tcp.h>
#include <functional>

#ifndef WASTLERNET_MODBUS_CONNECTION_H
#define WASTLERNET_MODBUS_CONNECTION_H
namespace wastlernet {
    /**
     * Base class managing a Modbus/TCP connection using libmodbus.
     *
     * After Init(), use Execute() to perform Modbus operations with an active modbus_t context.
     * The class can optionally read a small range of holding registers during initialization to
     * validate connectivity.
     */
    class ModbusConnection {
    public:
        /**
         * Construct a Modbus connection helper.
         * @param host        Target host (IPv4/IPv6/DNS name).
         * @param port        TCP port.
         * @param init_addr   Optional starting address for an initial read during Init() (-1 to skip).
         * @param init_count  Number of registers to read for the initial check.
         */
        ModbusConnection(const std::string& host, int16_t port, int32_t init_addr = -1, int16_t init_count = 2)
        : host_(host), port_(port), init_addr_(init_addr), init_count_(init_count) { }

        virtual ~ModbusConnection();

        /**
         * Initialize (or re-initialize) the libmodbus context and establish a TCP connection.
         * Safe to call multiple times; subsequent calls are no-ops if already initialized.
         * @return absl::OkStatus on success; a non-OK status describing the error otherwise.
         */
        absl::Status Init();

        /**
         * Execute user code with the active modbus_t context. Handles reconnects transparently
         * if the connection is lost and Reinit() succeeds.
         *
         * @param method A function that receives the active modbus_t* and returns status.
         * @return absl::OkStatus if the callback returned OK; otherwise a non-OK status.
         */
        absl::Status Execute(std::function<absl::Status(modbus_t*)> method);

    private:
        absl::Mutex mutex_;

        std::string host_;
        int16_t port_;
        int32_t init_addr_;
        int16_t init_count_;

        bool initialized_ = false;

        modbus_t *ctx_;

        /** Check if a reinit is necessary and re-establish the connection if needed. */
        absl::Status Reinit();

    protected:
        /** Name of the connection (for logging/diagnostics). Must be provided by derived classes. */
        virtual std::string Name() = 0;

    public:
        /** Convert a single 16-bit register to a signed int16_t. */
        static int16_t toInt16(const uint16_t* u);

        /** Convert two consecutive 16-bit registers (big-endian) to a signed int32_t. */
        static int32_t toInt32(const uint16_t* u);

        /** Convert four consecutive 16-bit registers (big-endian) to a signed int64_t. */
        static int64_t toInt64(const uint16_t* u);

        /**
         * Convert two consecutive 16-bit registers to a 32-bit float (IEEE 754).
         * No bounds checking is performed; ensure the buffer is large enough and endianness matches device spec.
         */
        static float toFloat(const uint16_t* u);

        /**
         * Convert the next 'nchars' bytes starting at the value pointed to by 'u' to a string.
         * No bounds checking; caller must ensure there is enough data.
         */
        static std::string toString(const uint16_t* u, int nchars);

        /** Interpret a single 16-bit register as a 16-bit bitset (bit 0 = LSB). */
        static std::bitset<16> toBitset16(const uint16_t* u);

        /** Interpret two consecutive 16-bit registers as a 32-bit bitset (bits 0..31). */
        static std::bitset<32> toBitset32(const uint16_t* u);
    };

}


#endif // WASTLERNET_MODBUS_CONNECTION_H
