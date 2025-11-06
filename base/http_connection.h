//
// Created by wastl on 04.07.25.
//
// HTTP connection helper.
//
// This header defines the wastlernet::HttpConnection base class which encapsulates the
// boilerplate for performing simple HTTP requests (GET/POST) against a fixed endpoint.
//
// Responsibilities
// - Lazily initialize and maintain a cpprestsdk http_client instance.
// - Serialize access and automatically re-initialize the client on failures.
// - Provide hook points to customize client configuration and request body.
// - Execute a user-provided callback with the received HTTP response.
//
// Thread-safety
// All public methods are internally synchronized via an absl::Mutex. Multiple threads may
// call Init() and Execute() concurrently on the same instance. The callback passed to Execute()
// is invoked while the connection is held; keep it short and non-blocking.
//
// Usage
// - Construct with base_url, path and request type.
// - Call Init() once before the first Execute(); subsequent calls are cheap no-ops.
// - Optionally override ClientConfig() to inject credentials, TLS verification, timeouts, etc.
// - For POST requests, optionally override RequestBody() to provide a JSON payload.
// - Provide a Name() in derived classes for logging/diagnostics.
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
    /**
     * Base class managing a simple HTTP endpoint interaction.
     *
     * A derived class specifies a human-readable Name(), and can customize the client
     * configuration and/or the request body for POST requests. After calling Init(),
     * use Execute() to perform the request and handle the response in the provided callback.
     */
    class HttpConnection {
      public:
        /** Kind of request to issue when calling Execute(). */
        enum RequestType {
            GET, POST
          };

        /**
         * Construct an HTTP connection helper.
         * @param base_url   The base URL (e.g. "http://host:port").
         * @param path       The path appended to the base URL (e.g. "/api/v1/resource").
         * @param request_type Type of request to perform (GET or POST).
         */
        HttpConnection(const std::string& base_url, const std::string& path, RequestType request_type)
          : base_url_(base_url), path_(path), request_type_(request_type) {}

        virtual ~HttpConnection() { }

        /**
         * Initialize the internal HTTP client.
         * Must be called before calling Execute(). Safe to call multiple times.
         * @return absl::OkStatus on success; a non-OK status describing the error otherwise.
         */
        absl::Status Init();

        /**
         * Execute the configured HTTP request and invoke the provided handler with the response.
         * The request method (GET/POST) is determined by the constructor argument, and for POST the
         * optional JSON body returned by RequestBody() is used if provided.
         *
         * @param method Callback receiving the HTTP response; return non-OK to signal an application error.
         * @return absl::OkStatus if the request succeeded and the callback returned OK; otherwise a non-OK status.
         */
        absl::Status Execute(std::function<absl::Status(const web::http::http_response&)> method);

      private:
        absl::Mutex mutex_;

        std::string base_url_, path_;
        RequestType request_type_;

        bool initialized_ = false;

        std::unique_ptr<web::http::client::http_client> client_;

        /** (Re-)initialize the client when necessary. */
        absl::Status Reinit();

      protected:
        /** Name of the connection (for logging/diagnostics). Must be provided by derived classes. */
        virtual std::string Name() = 0;

        /**
         * Override to customize the HTTP client configuration, e.g. credentials, proxy, TLS cert validation,
         * timeouts, etc.
         */
        virtual web::http::client::http_client_config ClientConfig() { return web::http::client::http_client_config(); }

        /**
         * Override to provide a JSON request body for POST requests.
         * Return std::nullopt for no body / GET requests.
         */
        virtual std::optional<web::json::value> RequestBody() { return std::nullopt; }
    };
}
#endif //HTTP_CONNECTION_H
