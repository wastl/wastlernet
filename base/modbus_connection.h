//
// Created by wastl on 03.07.25.
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
    // Boilerplate to manage a Modbus connection. Initializes and keeps track of a modbus_t context that can be used
    // when calling Execute()
    class ModbusConnection {
    public:
        ModbusConnection(const std::string& host, int16_t port, int32_t init_addr = -1, int16_t init_count = 2)
        : host_(host), port_(port), init_addr_(init_addr), init_count_(init_count) { }

        virtual ~ModbusConnection();

        // Initialize Modbus Context. Must be called before using Execute.
        absl::Status Init();

        // Execute the function passed as argument using the (established) modbus context managed by this class.
        absl::Status Execute(std::function<absl::Status(modbus_t*)> method);

    private:
        absl::Mutex mutex_;

        std::string host_;
        int16_t port_;
        int32_t init_addr_;
        int16_t init_count_;

        bool initialized_ = false;

        modbus_t *ctx_;

        // Check if a reinit is necessary and re-establish the connection if needed.
        absl::Status Reinit();

    protected:
        virtual std::string Name() = 0;

    public:
        // Convert the modbus register value pointed at by u to a signed int16_t.
        static int16_t toInt16(const uint16_t* u);

        static int32_t toInt32(const uint16_t* u);

        static int64_t toInt64(const uint16_t* u);

        // Convert the modbus register value pointed at by u to a 32bit float. Will read two consecutive registers.
        // No bounds checking.
        static float toFloat(const uint16_t* u);

        // Convert the next nchars values starting at the value pointed at by u to a string. No bounds checking.
        static std::string toString(const uint16_t* u, int nchars);

        static std::bitset<16> toBitset16(const uint16_t* u);

        static std::bitset<32> toBitset32(const uint16_t* u);
    };

}


#endif // WASTLERNET_MODBUS_CONNECTION_H
