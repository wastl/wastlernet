//
// Created by wastl on 02.04.23.
//
#include <glog/logging.h>

#include "solvis_client.h"

int main(int argc, char *argv[]) {
    google::InitGoogleLogging(argv[0]);

    solvis::query("192.168.178.40", 502, [](const solvis::SolvisData& data) {
        std::cout << "Debug: " << data.DebugString() << std::endl;
    });
}