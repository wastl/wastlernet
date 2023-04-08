//
// Created by wastl on 02.04.23.
//

#ifndef WASTLERNET_SOLVIS_CLIENT_H
#define WASTLERNET_SOLVIS_CLIENT_H

#include <functional>
#include <string>
#include <absl/strings/string_view.h>
#include <absl/status/status.h>

#include "solvis/solvis.pb.h"


namespace solvis {
absl::Status query(const absl::string_view host, int port, const std::function<void(const SolvisData&)>& handler);
}

#endif //WASTLERNET_SOLVIS_CLIENT_H
