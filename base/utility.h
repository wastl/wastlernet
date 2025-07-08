//
// Created by wastl on 27.10.23.
//
#include <absl/status/status.h>

#ifndef WASTLERNET_UTILITY_H
#define WASTLERNET_UTILITY_H
namespace wastlernet {
    absl::Status retry_with_backoff(std::function<absl::Status()> method, int times);
}
#endif //WASTLERNET_UTILITY_H
