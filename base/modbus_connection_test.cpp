//
// Created by wastl on 04.07.25.
//
#include <thread>
#include <gtest/gtest.h>
#include <modbus/modbus.h>

#include "modbus_connection.h"

#include <glog/logging.h>

class ModbusTest : public testing::Test {
protected:
    ModbusTest() {
    }

    void SetUp() override {
        // Code here will be called immediately after the constructor (right
        // before each test).

        server_thread_.reset(new std::thread([=]() {
            ASSERT_EQ(this->runModbusTCPServer(15002, 50), 0);
        }));

        // Give the server a moment to start up and bind to the port
        while (!running_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    void TearDown() override {
        // Code here will be called immediately after each test (right
        // before the destructor).
        running_ = false;

        if (server_thread_ != nullptr) {
            server_thread_->join();
        }
    }

private:
    bool running_ = false;
    std::unique_ptr<std::thread> server_thread_;

    int runModbusTCPServer(int port, int num_registers) {
        modbus_t *ctx = nullptr;
        modbus_mapping_t *mb_mapping = nullptr;
        int server_socket = -1;

        // 1. Create a Modbus TCP context
        // The IP address is NULL, which means it will listen on all available network interfaces.
        ctx = modbus_new_tcp(NULL, port);
        if (ctx == NULL) {
            std::cerr << "Failed to create Modbus TCP context: " << modbus_strerror(errno) << std::endl;
            return -1;
        }

        // Set Modbus debug mode (optional, useful for debugging)
        // modbus_set_debug(ctx, TRUE);

        // 2. Allocate Modbus mapping structure
        // This structure holds the actual register data.
        // We'll set the number of coils, discrete inputs, holding registers, and input registers.
        // For this example, we'll focus on holding registers.
        mb_mapping = modbus_mapping_new(
            10, // num_coils (0x01, 0x02)
            10, // num_discrete_inputs (0x02)
            num_registers, // num_holding_registers (0x03, 0x06, 0x10)
            10 // num_input_registers (0x04)
        );

        if (mb_mapping == NULL) {
            std::cerr << "Failed to allocate Modbus mapping: " << modbus_strerror(errno) << std::endl;
            modbus_free(ctx);
            return -1;
        }

        // Initialize holding registers with some sample data
        for (int i = 0; i < num_registers; ++i) {
            mb_mapping->tab_registers[i] = i + 100; // Example data
        }
        std::cout << "Initialized " << num_registers << " holding registers." << std::endl;

        // 3. Bind the server to the specified port
        // This makes the server listen for incoming connections.
        server_socket = modbus_tcp_listen(ctx, 1); // 1 is the maximum number of pending connections
        if (server_socket == -1) {
            std::cerr << "Failed to listen on TCP port " << port << ": " << modbus_strerror(errno) << std::endl;
            modbus_mapping_free(mb_mapping);
            modbus_free(ctx);
            return -1;
        }
        std::cout << "Modbus TCP server listening on port " << port << std::endl;

        running_ = true;
        // 4. Main server loop to handle client connections
        while (running_) {
            // Accept a new client connection
            int client_socket = modbus_tcp_accept(ctx, &server_socket);
            if (client_socket == -1) {
                std::cerr << "Failed to accept client connection: " << modbus_strerror(errno) << std::endl;
                // Depending on the error, you might want to break or continue
                continue;
            }
            std::cout << "Client connected (socket: " << client_socket << ")" << std::endl;

            // Set the socket for the Modbus context to the newly accepted client socket
            modbus_set_socket(ctx, client_socket);

            // Buffer for Modbus requests
            uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];
            int rc; // Return code

            // Loop to serve requests from the current client
            while (running_) {
                // Read the request from the client
                rc = modbus_receive(ctx, query);
                if (rc == -1) {
                    // Connection closed by client or an error occurred
                    std::cerr << "Client connection closed or error: " << modbus_strerror(errno) << std::endl;
                    break; // Exit inner loop to accept new client
                } else if (rc == 0) {
                    // No request received (e.g., timeout, though modbus_receive blocks)
                    std::cout << "No request received from client, perhaps timeout." << std::endl;
                    continue;
                }

                // Process the Modbus request and send the response
                // modbus_reply handles the request using the data in mb_mapping
                modbus_reply(ctx, query, rc, mb_mapping);
                std::cout << "Served request from client." << std::endl;
            }

            // Close the client socket when done with it
            close(client_socket);
            std::cout << "Client disconnected (socket: " << client_socket << ")" << std::endl;
        }

        // 5. Cleanup (This part is typically unreachable in an infinite server loop,
        // but essential if the loop can terminate.)
        std::cout << "Server shutting down..." << std::endl;
        modbus_mapping_free(mb_mapping);
        modbus_free(ctx);
        if (server_socket != -1) {
            close(server_socket);
        }

        return 0;
    }
};

class TestModbusConnection : public wastlernet::ModbusConnection {
public:
    TestModbusConnection()
        : ModbusConnection("127.0.0.1", 15002, 0, 2) {
    }

protected:
    std::string Name() override {
        return "TestModbusConnection";
    }
};

// Demonstrate some basic assertions.
TEST_F(ModbusTest, ReadRegisters) {
    TestModbusConnection test;

    ASSERT_TRUE(test.Init().ok());

    ASSERT_TRUE(test.Execute([](modbus_t* ctx) {
        LOG(INFO) << "Reading Modbus registers";

        uint16_t tab_reg[50];
        int rc = modbus_read_registers(ctx, 0, 50, tab_reg);

        if (rc == -1) {
            LOG(ERROR) << "Error reading register: " << modbus_strerror(errno);
            return absl::InternalError(std::string("Error reading register: ") + modbus_strerror(errno));
        }

        std::cout << "Register values:" << std::endl;
        for (int i = 0; i < 50; ++i) {
            std::cout << tab_reg[i] << " ";
            EXPECT_EQ(tab_reg[i], i+100);
        }
        std::cout << std::endl;

        return absl::OkStatus();
    }).ok());
}

TEST_F(ModbusTest, WriteRegisters) {
    TestModbusConnection test;

    ASSERT_TRUE(test.Init().ok());

    ASSERT_TRUE(test.Execute([](modbus_t* ctx) {
        LOG(INFO) << "Writing Modbus registers";

        int addr = 5;
        uint16_t value = 500;
        int rc = modbus_write_register(ctx, addr, value);

        if (rc == -1) {
            LOG(ERROR) << "Error writing register: " << modbus_strerror(errno);
            return absl::InternalError(std::string("Error writing register: ") + modbus_strerror(errno));
        }

        uint16_t read_back_value;
        rc = modbus_read_registers(ctx, addr, 1, &read_back_value);
        if (rc == -1) {
            LOG(ERROR) << "Error reading register: " << modbus_strerror(errno);
            return absl::InternalError(std::string("Error reading register: ") + modbus_strerror(errno));
        }

        EXPECT_EQ(read_back_value, value);

        return absl::OkStatus();
    }).ok());
}

TEST_F(ModbusTest, TypeConversions) {
    TestModbusConnection test;
    ASSERT_TRUE(test.Init().ok());

    uint16_t value1 = 500;
    EXPECT_EQ(wastlernet::ModbusConnection::toInt16(&value1), 500);

    uint16_t value2 = INT16_MAX+1;
    EXPECT_EQ(wastlernet::ModbusConnection::toInt16(&value2), -INT16_MAX-1);

    uint16_t value3[2] = {0, 500};
    EXPECT_EQ(wastlernet::ModbusConnection::toInt32(value3), 500);

    uint16_t value4[2] = {0, INT16_MAX+1};
    EXPECT_EQ(wastlernet::ModbusConnection::toInt32(value4), INT16_MAX+1);

    uint16_t value5[2] = {1, 0};
    EXPECT_EQ(wastlernet::ModbusConnection::toInt32(value5), 2*(INT16_MAX+1));

    uint16_t value6[5] = {'H', 'e', 'l', 'l', 'o'};
    EXPECT_EQ(wastlernet::ModbusConnection::toString(value6, 5), "Hello");

    // TODO: test with Sunspec values
    uint16_t value7[2] = {0x47F1, 0x2000};
    EXPECT_DOUBLE_EQ(wastlernet::ModbusConnection::toFloat(value7), 123456.0);
}
