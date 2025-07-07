//
// Created by wastl on 07.07.24.
//
#include <functional>
#include <string>
#include <absl/strings/string_view.h>
#include <absl/status/status.h>

#include "config/config.pb.h"
#include "hue/hue.pb.h"

#ifndef WASTLERNET_HUE_CLIENT_H
#define WASTLERNET_HUE_CLIENT_H
namespace hue {
    absl::Status query(const wastlernet::Hue& hueCfg, const std::function<absl::Status(const HueData&)>& handler);
}
#endif //WASTLERNET_HUE_CLIENT_H
