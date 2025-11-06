#include <iostream>
#include <thread>
#include <glog/logging.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/util/json_util.h>
#include <fcntl.h>
#include <absl/strings/str_split.h>
#include <absl/strings/strip.h>
#include <absl/strings/str_cat.h>
#include <absl/status/status.h>
#include <absl/status/statusor.h>
#include <cpprest/http_listener.h>
#include <prometheus/exposer.h>

#include "base/metrics.h"
#include "base/utility.h"
#include "config/config.pb.h"
#include "fronius/fronius_client.h"
#include "fronius/fronius_module.h"

#include "hafnertec/hafnertec_module.h"
#include "senec/senec_module.h"
#include "solvis/solvis_modbus.h"
#include "solvis/solvis_updater.h"
#include "solvis/solvis_module.h"
#include "weather/weather_module.h"

using namespace std::chrono;
using google::protobuf::io::ZeroCopyInputStream;
using google::protobuf::io::FileInputStream;
using namespace google::protobuf::util;
using namespace std::chrono_literals;

using web::http::http_request;
using web::http::experimental::listener::http_listener;

namespace wastlernet {
    namespace rest {
        template<class T>
        void ParseAndConvert(const std::string &binary_message, std::string *output) {
            JsonPrintOptions options;
            options.add_whitespace = true;

            T value;
            value.ParseFromString(binary_message);
            MessageToJsonString(value, output, options);
        }

        // Start a listener serving the current state of different modules as JSON.
        std::unique_ptr<http_listener>
        start_listener(const std::string &listen, const wastlernet::StateCache *stateCache) {
            auto listener = std::make_unique<http_listener>(listen);

            LOG(INFO) << "starting REST listener on address " << listen;

            listener->support([=](const http_request &request) {
                auto uri = request.relative_uri();

                LOG(INFO) << "Received REST request to " << uri.path();

                std::string path = std::string(absl::StripPrefix(uri.path(), "/"));

                try {
                    auto it = stateCache->find(path);
                    if (it != stateCache->cend()) {
                        std::string binary_message = it->second;
                        std::string output;

                        if (path == "weather") {
                            ParseAndConvert<weather::WeatherData>(binary_message, &output);
                        } else if (path == "solvis") {
                            ParseAndConvert<solvis::SolvisData>(binary_message, &output);
                        } else if (path == "senec") {
                            ParseAndConvert<senec::SenecData>(binary_message, &output);
                        } else if (path == "hafnertec") {
                            ParseAndConvert<hafnertec::HafnertecData>(binary_message, &output);
                        } else {
                            throw std::runtime_error(absl::StrCat("unsupported module: ", path));
                        }

                        request.reply(web::http::status_codes::OK, output, "text/json").get();
                    } else {
                        request.reply(web::http::status_codes::NotFound, absl::StrCat("data for module \"", path, "\" not found"), "text/plain").get();
                    }
                } catch (const std::exception &e) {
                    LOG(ERROR) << "Error while resolving REST request: " << e.what();
                    request.reply(web::http::status_codes::InternalError);
                }
            });

            listener->open().get();

            return std::move(listener);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: wastlernet <path-to-config.yml>" << std::endl;
        return 1;
    }

    google::InitGoogleLogging(argv[0]);
    google::EnableLogCleaner(24h * 3);


    wastlernet::Config config;

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        LOG(ERROR) << "Could not open configuration file";
        return 1;
    }
    ZeroCopyInputStream* cfg_file = new FileInputStream(fd);
    if (!google::protobuf::TextFormat::Parse(cfg_file, &config)) {
        LOG(ERROR) << "Unable to parse configuration file" << std::endl;
        return 1;
    }
    delete cfg_file;
    close(fd);

    LOG(INFO) << "Loaded configuration." << std::endl;

    std::string prometheus_address;
    if (config.has_prometheus() && config.prometheus().has_listen()) {
        prometheus_address = config.prometheus().listen();
    } else {
        prometheus_address = "127.0.0.1:32154";
    }
    prometheus::Exposer exposer{prometheus_address};
    exposer.RegisterCollectable(wastlernet::metrics::WastlernetMetrics::GetInstance().registry);


    wastlernet::StateCache current_state;
    std::vector<std::unique_ptr<wastlernet::IModule>> modules;

    std::unique_ptr<solvis::SolvisModbusConnection> solvis_connection;
    if (config.has_solvis()) {
        solvis_connection = std::make_unique<solvis::SolvisModbusConnection>(config.solvis());
        auto solvis_st = solvis_connection->Init();
        if (!solvis_st.ok()) {
            LOG(ERROR) << "Could not initialize Solvis connection: " << solvis_st;
            return 1;
        }

        auto solvis_client = std::make_unique<solvis::SolvisModule>(config.timescaledb(),config.solvis(), solvis_connection.get(), &current_state);
        solvis_st = solvis_client->Init();
        if (!solvis_st.ok()) {
            LOG(ERROR) << "Could not initialize Solvis module: " << solvis_st;
            return 1;
        }
        solvis_client->Start();
        modules.emplace_back(std::move(solvis_client));

        LOG(INFO) << "Started Solvis module." << std::endl;

        solvis::SolvisUpdater solvis_updater(config.solvis(), solvis_connection.get(), &current_state);
        auto su_st = solvis_updater.Init();
        if(!su_st.ok()) {
            LOG(ERROR) << "Could not initialize Solvis updater: " << su_st;
            return 1;
        }
        solvis_updater.Start();

        LOG(INFO) << "Started Solvis updater." << std::endl;
    }

    if (config.has_hafnertec()) {
        auto hafnertec_client = std::make_unique<hafnertec::HafnertecModule>(config.timescaledb(), config.hafnertec(), &current_state);
        auto hafnertec_st = hafnertec_client->Init();
        if(!hafnertec_st.ok()) {
            LOG(ERROR) << "Could not initialize Hafnertec module: " << hafnertec_st;
            return 1;
        }
        hafnertec_client->Start();
        modules.emplace_back(std::move(hafnertec_client));

        LOG(INFO) << "Started Hafnertec module." << std::endl;
    }

    if (config.has_senec()) {
        auto senec_client = std::make_unique<senec::SenecModule>(config.timescaledb(), config.senec(), &current_state);
        auto senec_st = senec_client->Init();
        if(!senec_st.ok()) {
            LOG(ERROR) << "Could not initialize Senec module: " << senec_st;
            return 1;
        }
        senec_client->Start();
        modules.emplace_back(std::move(senec_client));

        LOG(INFO) << "Started Fronius module." << std::endl;
    }

    if (config.has_fronius()) {
        auto fronius_client = std::make_unique<fronius::FroniusModule>(config.timescaledb(), config.fronius(), &current_state);
        auto fronius_st = fronius_client->Init();
        if(!fronius_st.ok()) {
            LOG(ERROR) << "Could not initialize Fronius module: " << fronius_st;
            return 1;
        }
        fronius_client->Start();
        modules.emplace_back(std::move(fronius_client));

        LOG(INFO) << "Started Fronius module." << std::endl;
    }

    if (config.has_weather()) {
        auto weather_client = std::make_unique<weather::WeatherModule>(config.timescaledb(), config.weather(), &current_state);
        auto weather_st = weather_client->Init();
        if(!weather_st.ok()) {
            LOG(ERROR) << "Could not initialize Weather module: " << weather_st;
            return 1;
        }
        weather_client->Start();
        modules.emplace_back(std::move(weather_client));

        LOG(INFO) << "Started Weather module." << std::endl;
    }

    auto rest_listener = wastlernet::rest::start_listener(config.rest().listen(), &current_state);

    LOG(INFO) << "Smart Home Controller startup sequence completed.";

    for (auto &module : modules) {
        module->Wait();
    }
}
