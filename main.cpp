#include <iostream>
#include <thread>
#include <chrono>
#include <glog/logging.h>
#include <fstream>
#include <google/protobuf/text_format.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <fcntl.h>
#include <absl/status/status.h>

#include "config/config.pb.h"

#include "hafnertec/hafnertec_module.h"
#include "senec/senec_module.h"
#include "solvis/solvis_module.h"

using namespace std::chrono;
using google::protobuf::io::ZeroCopyInputStream;
using google::protobuf::io::FileInputStream;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: wastlernet <path-to-config.yml>" << std::endl;
        return 1;
    }

    google::InitGoogleLogging(argv[0]);

    wastlernet::Config config;

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        std::cerr << "Could not open configuration file";
        return 1;
    }
    ZeroCopyInputStream* cfg_file = new FileInputStream(fd);
    if (!google::protobuf::TextFormat::Parse(cfg_file, &config)) {
        std::cerr << "Unable to parse configuration file" << std::endl;
        return 1;
    }
    delete cfg_file;
    close(fd);

    std::cout << "Loaded configuration." << std::endl;

    solvis::SolvisModule solvis_client(config.timescaledb(),config.solvis());
    auto solvis_st = solvis_client.Init();
    if (!solvis_st.ok()) {
        std::cerr << "Could not initialize Solvis module: " << solvis_st;
        return 1;
    }
    solvis_client.Start();

    std::cout << "Started Solvis module." << std::endl;

    hafnertec::HafnertecModule hafnertec_client(config.timescaledb(), config.hafnertec());
    auto hafnertec_st = hafnertec_client.Init();
    if(!hafnertec_st.ok()) {
        std::cerr << "Could not initialize Hafnertec module: " << hafnertec_st;
        return 1;
    }
    hafnertec_client.Start();

    std::cout << "Started Hafnertec module." << std::endl;

    senec::SenecModule senec_client(config.timescaledb(), config.senec());
    auto senec_st = senec_client.Init();
    if(!senec_st.ok()) {
        std::cerr << "Could not initialize Senec module: " << senec_st;
        return 1;
    }
    senec_client.Start();

    std::cout << "Started Senec module." << std::endl;

    solvis_client.Wait();
    hafnertec_client.Wait();
    senec_client.Wait();
}