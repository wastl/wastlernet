//
// Created by wastl on 18.01.22.
//
#include <functional>
#include <absl/status/status.h>

#include "include/http_connection.h"
#include "senec/senec.pb.h"

#ifndef SENEC_EXPORTER_SENEC_CLIENT_H
#define SENEC_EXPORTER_SENEC_CLIENT_H

namespace senec {
     absl::Status query(const std::string& uri, const std::function<void(const SenecData&)>& handler);

     class SenecClient : public wastlernet::HttpConnection {
     public:
          SenecClient(const std::string &base_url)
               : HttpConnection(base_url, "/lala.cgi", POST) { }

          absl::Status Query(const std::function<void(const SenecData&)>& handler);

     protected:
          std::string Name() override { return "SenecClient"; }

          web::http::client::http_client_config ClientConfig() override;

          std::optional<web::json::value> RequestBody() override;
     };
}

#endif //SENEC_EXPORTER_SENEC_CLIENT_H
