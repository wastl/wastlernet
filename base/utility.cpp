//
// Created by wastl on 27.10.23.
//
#include "utility.h"

#include <thread>

namespace wastlernet {
    absl::Status retry_with_backoff(std::function<absl::Status()> method, int times) {
        absl::Status st;
        for (int retries = 0; retries < times; retries++) {
            st = method();
            if (st.ok()) {
                return st;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100*(retries+1)));
        }
        return st;
    }
}