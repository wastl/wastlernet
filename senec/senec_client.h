//
// Created by wastl on 18.01.22.
//
#include <functional>
#include "senec/senec.pb.h"

#ifndef SENEC_EXPORTER_SENEC_CLIENT_H
#define SENEC_EXPORTER_SENEC_CLIENT_H

namespace senec {

    void query(const std::string& uri, const std::function<void(const SenecData&)>& handler);
}

#endif //SENEC_EXPORTER_SENEC_CLIENT_H
