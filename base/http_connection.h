//
// Created by wastl on 04.07.25.
//
#include <optional>
#include <string>
#include <absl/status/status.h>
#include <absl/synchronization/mutex.h>
#include <cpprest/uri.h>                        // URI library
#include <cpprest/http_msg.h>
#include <cpprest/http_client.h>
#include <cpprest/json.h>

#ifndef HTTP_CONNECTION_H
#define HTTP_CONNECTION_H

namespace wastlernet {
    class HttpConnection {
      public:
        enum RequestType {
            GET, POST
          };

        HttpConnection(const std::string& base_url, const std::string& path, RequestType request_type)
          : base_url_(base_url), path_(path), request_type_(request_type) {}

        virtual ~HttpConnection() { }


        // Initialize Modbus Context. Must be called before using Execute.
        absl::Status Init();

        // Execute the function passed as argument using the (established) modbus context managed by this class.
        absl::Status Execute(std::function<absl::Status(const web::http::http_response&)> method);

      private:
        absl::Mutex mutex_;

        std::string base_url_, path_;
        RequestType request_type_;

        bool initialized_ = false;

        std::unique_ptr<web::http::client::http_client> client_;

        absl::Status Reinit();

      protected:
        virtual std::string Name() = 0;

        // Override to use a different client config for HTTP requests, e.g. provide credentials or SSL certificate
        // validation.
        virtual web::http::client::http_client_config ClientConfig() { return web::http::client::http_client_config(); }

        // Override to use a different request body for a POST request
        virtual std::optional<web::json::value> RequestBody() { return std::nullopt; }
    };
}
#endif //HTTP_CONNECTION_H
