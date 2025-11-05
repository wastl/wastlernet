//
// Created by wastl on 04.07.25.
//

#include "http_connection.h"

#include <glog/logging.h>
#include <absl/strings/str_cat.h>
#include <optional>

#define LOGS(level) LOG(level) << "[" << Name() << "] "

using namespace web::http::client;
using web::http::methods;
using web::http::status_codes;
using web::http::http_response;

namespace wastlernet {
    absl::Status HttpConnection::Init() {
        absl::MutexLock lock(&mutex_);

        LOGS(INFO) << "Initializing and testing HTTP connection";
        client_.reset(new http_client(base_url_, ClientConfig()));

        try {
            return client_->request(methods::GET).then(
                [=](const http_response &response) {
                    if (response.status_code() == status_codes::OK) {
                        initialized_ = true;
                        return absl::OkStatus();
                    } else {
                        LOGS(ERROR) << "HTTP test request failed: " << response.reason_phrase();
                        return absl::FailedPreconditionError(
                            absl::StrCat("HTTP test request failed: ", response.reason_phrase()));
                    }
                }).get();
        } catch (const std::exception &e) {
            LOGS(ERROR) << "Error while testing HTTP connection: " << e.what();
            return absl::InternalError(absl::StrCat("Error while testing HTTP connection: ", e.what()));
        }
    }

    absl::Status HttpConnection::Execute(std::function<absl::Status(const http_response &)> handler) {
        LOGS(INFO) << "Executing HTTP request";

        if (!initialized_) {
            return absl::FailedPreconditionError("HTTP connection not initialized");
        }

        // Build request URI and start the request.
        web::uri_builder builder(U(path_));

        absl::MutexLock lock(&mutex_);
        try {
            if (request_type_ == GET) {
                return client_->request(methods::GET, builder.to_string()).then(handler).get();
            }
            if (request_type_ == POST) {
                std::optional<web::json::value> body = RequestBody();
                if (body.has_value()) {
                    return client_->request(methods::POST, builder.to_string(), *body).then(handler).get();
                } else {
                    return client_->request(methods::POST, builder.to_string()).then(handler).get();
                }
            }
            LOGS(ERROR) << "Unknown HTTP request type";
            return absl::InternalError("Unknown HTTP request type");
        } catch (const std::exception &e) {
            LOGS(ERROR) << "Error while executing HTTP request: " << e.what();
            return absl::InternalError(absl::StrCat("Error while executing HTTP request: ", e.what()));
        }
    }

    absl::Status HttpConnection::Reinit() {
        return Init();
    }
} // namespace
