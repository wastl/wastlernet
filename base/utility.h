//
// Created by wastl on 27.10.23.
//
#pragma once
#include <absl/status/status.h>

#ifndef WASTLERNET_UTILITY_H
#define WASTLERNET_UTILITY_H
namespace wastlernet {
    absl::Status retry_with_backoff(const std::function<absl::Status()>& method, int times);
}

#define RETURN_IF_ERROR(call) { \
    auto call_st = call; \
    if (!call_st.ok()) { \
        LOG(ERROR) << "Call failed: " << call_st; \
        return call_st; \
    }}
#endif //WASTLERNET_UTILITY_H
