//
// Created by wastl on 07.04.23.
//
#include <chrono>
#include <thread>
#include <absl/status/status.h>

#include "config/config.pb.h"
#include "timescaledb/timescaledb-client.h"

#ifndef WASTLERNET_MODULE_H
#define WASTLERNET_MODULE_H
namespace wastlernet {
    template<class Data>
    class Module {
    protected:
        timescaledb::TimescaleConnection<Data> conn_;

        absl::Status Update(const Data& data) {
            return conn_.Update(data);
        }

    public:
        Module(const TimescaleDB& config, timescaledb::TimescaleWriter<Data>* writer)
                : conn_(writer, config.database(), config.host(), config.port(), config.user(), config.password()) {
        }

        // Perform initialization needed before starting.
        virtual absl::Status Init() {
            return conn_.Init();
        }

        virtual void Start() = 0;

        virtual void Abort() = 0;

        virtual void Wait() = 0;
    };

    template<class Data>
    class PollingModule : public Module<Data> {
    private:
        int poll_interval;
        bool aborted_;
        std::thread t_;

    protected:
        virtual absl::Status Query(std::function<absl::Status(const Data& data)> handler) = 0;

    public:
        PollingModule(const TimescaleDB& config, timescaledb::TimescaleWriter<Data>* writer, int poll_interval)
        : Module<Data>(config, writer), poll_interval(poll_interval), aborted_(false) { }

        void Start() override {
            t_ = std::thread([this]() {
                auto next = std::chrono::system_clock::now();
                while (!aborted_) {
                    next += std::chrono::seconds(poll_interval);
                    std::this_thread::sleep_until(next);
                    auto st = Query([this](const Data& data){
                        auto st = Module<Data>::Update(data);
                        if (!st.ok()) {
                            LOG(ERROR) << "Error writing to database: " << st;
                        }
                        return st;
                    });
                    if (!st.ok()) {
                        LOG(ERROR) << "Error: " << st;
                    }
                }
            });
        }

        void Abort() override {
            aborted_ = true;
            t_.join();
        }

        void Wait() override {
            t_.join();
        }
    };
}
#endif //WASTLERNET_MODULE_H
