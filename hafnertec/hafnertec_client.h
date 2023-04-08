//
// Created by wastl on 09.12.22.
//
#include <functional>
#include <string>
#include <absl/strings/string_view.h>

#include "hafnertec/hafnertec.pb.h"

#ifndef HAFNERTEC_EXPORTER_HAFNERTEC_CLIENT_H
#define HAFNERTEC_EXPORTER_HAFNERTEC_CLIENT_H
namespace hafnertec {
     void query(absl::string_view uri, absl::string_view user, absl::string_view password, const std::function<void(const HafnertecData&)>& handler);
}



#endif //HAFNERTEC_EXPORTER_HAFNERTEC_CLIENT_H
