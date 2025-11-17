//
// Created by wastl on 27.10.23.
//
#pragma once
#include <chrono>
#include <thread>
#include <absl/status/status.h>
#include <absl/container/flat_hash_map.h>
#include <glog/logging.h>

#include "config/config.pb.h"
#ifndef WASTLERNET_UPDATER_H
#define WASTLERNET_UPDATER_H
namespace wastlernet {
    typedef absl::flat_hash_map<std::string, std::string> StateCache;

    template<class Data>
    class Updater {
    protected:
        // not owned
        StateCache* current_state_;

        virtual absl::Status Update() = 0;

    public:
        Updater(StateCache* current_state)
                : current_state_(current_state) {
        }

        // Perform initialization needed before starting.
        virtual absl::Status Init() {
            return absl::OkStatus();
        }

        virtual void Start() = 0;

        virtual void Abort() = 0;

        virtual void Wait() = 0;
    };

    template<class Data>
    class PollingUpdater : public Updater<Data> {
    private:
        int poll_interval;
        bool aborted_;
        std::thread t_;

    public:
        PollingUpdater(StateCache* current_state, int poll_interval)
                : Updater<Data>(current_state), poll_interval(poll_interval), aborted_(false) { }

        void Start() override {
            t_ = std::thread([this]() {
                auto next = std::chrono::system_clock::now();
                while (!aborted_) {
                    next += std::chrono::seconds(poll_interval);
                    std::this_thread::sleep_until(next);
                    auto st = this->Update();
                    if (!st.ok()) {
                        LOG(ERROR) << "Updater Error: " << st;
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
#endif //WASTLERNET_UPDATER_H
