//
// Created by wastl on 27.10.23.
//
#include "utility.h"

#include <thread>
#include <chrono>

namespace wastlernet {
    absl::Status retry_with_backoff(const std::function<absl::Status()>& method, int times) {
        absl::Status st;
        for (int retries = 0; retries < times; retries++) {
            st = method();
            if (st.ok()) {
                return st;
            }
            // Exponential backoff: 100ms * 2^retries
            const auto delay_ms = 100 * (1 << retries);
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
        }
        return st;
    }
}