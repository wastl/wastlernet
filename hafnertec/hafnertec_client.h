//
// Created by wastl on 09.12.22.
//
#include <functional>
#include <string>
#include <absl/strings/string_view.h>
#include <absl/status/status.h>

#include "include/http_connection.h"
#include "hafnertec/hafnertec.pb.h"

#ifndef HAFNERTEC_EXPORTER_HAFNERTEC_CLIENT_H
#define HAFNERTEC_EXPORTER_HAFNERTEC_CLIENT_H

namespace hafnertec {
    class HafnertecClient : public wastlernet::HttpConnection {
    public:
        HafnertecClient(const std::string &base_url, const std::string &user, const std::string &password)
            : HttpConnection(base_url, "/schematic_files/9.cgi", GET),
              user_(user), password_(password) {
        }

        absl::Status Query(const std::function<void(const HafnertecData &)> &handler);

    protected:
        std::string Name() override { return "HafnertecClient"; }

        web::http::client::http_client_config ClientConfig() override;

    private:
        std::string user_, password_;
    };
}


#endif //HAFNERTEC_EXPORTER_HAFNERTEC_CLIENT_H
