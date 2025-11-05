//
// Created by wastl on 06.07.25.
//
#include <thread>
#include <gtest/gtest.h>

#include "http_connection.h"

#include <absl/strings/strip.h>
#include <cpprest/http_listener.h>
#include <glog/logging.h>

using web::http::http_request;
using web::http::experimental::listener::http_listener;

constexpr char kAddress[] = "http://127.0.0.1:15003";

class HTTPTest : public testing::Test {
protected:
    HTTPTest() {
    }

    void SetUp() override {
        server_thread_ = startListener(kAddress);
    }

    void TearDown() override {
        if (server_thread_ != nullptr) {
            server_thread_->close();
        }
    }

private:
    std::unique_ptr<http_listener> server_thread_;


    // Start a listener for test purposes that echos back the request as JSON
    std::unique_ptr<http_listener> startListener(const std::string &listen) {
        auto listener = std::make_unique<http_listener>(listen);

        LOG(INFO) << "starting REST listener on address " << listen;

        listener->support([=](const http_request &request) {
            auto uri = request.relative_uri();

            LOG(INFO) << "Received REST request to " << uri.path();

            std::string path = std::string(absl::StripPrefix(uri.path(), "/"));

            try {
                if (path == "") {
                    // empty path is connection init
                    request.reply(web::http::status_codes::OK).get();
                } else if (path == "test") {
                    // /test path is used for testing GET and POST
                    web::json::value output = web::json::value::object({
                        {"method", web::json::value::string(request.method())},
                        {"path", web::json::value::string(uri.path())},
                        {"body", request.extract_json().get()}
                    });

                    request.reply(web::http::status_codes::OK, output).get();
                } else {
                    // everything else should return NOT FOUND
                    request.reply(web::http::status_codes::NotFound,
                                  std::string("data for module \"") + path + "\" not found", "text/plain").get();
                }
            } catch (const std::exception &e) {
                LOG(ERROR) << "Error while resolving REST request: " << e.what();
                request.reply(web::http::status_codes::InternalError);
            }
        });

        listener->open().get();

        return std::move(listener);
    }
};

// Test implementations of wastlernet::HttpConnection for different methods and paths
namespace {
    class TestHTTPGetConnection : public wastlernet::HttpConnection {
    public:
        TestHTTPGetConnection()
            : HttpConnection(kAddress, "test", GET) {
        }

    protected:
        std::string Name() override { return "TestHTTPGetConnection"; }
    };

    class TestHTTPPostConnection : public wastlernet::HttpConnection {
    public:
        TestHTTPPostConnection()
            : HttpConnection(kAddress, "test", POST) {
        }

    protected:
        std::string Name() override { return "TestHTTPPostConnection"; }

        std::optional<web::json::value> RequestBody() override {
            return web::json::value::object({
                {"foo", web::json::value::string("bar")}
            });
        }
    };

    class TestHTTPNotFoundConnection : public wastlernet::HttpConnection {
    public:
        TestHTTPNotFoundConnection()
            : HttpConnection(kAddress, "not-found", GET) {
        }

    protected:
        std::string Name() override { return "TestHTTPNotFoundConnection"; }
    };
}

// Test initialization and GET request
TEST_F(HTTPTest, Get) {
    TestHTTPGetConnection conn;

    auto st = conn.Init();
    ASSERT_TRUE(st.ok()) << st.message();

    st = conn.Execute([](const web::http::http_response &response) {
        auto json = response.extract_json().get();

        EXPECT_EQ("GET", json.at("method").as_string());
        EXPECT_EQ("/test", json.at("path").as_string());

        return absl::OkStatus();
    });
    ASSERT_TRUE(st.ok()) << st.message();
}

// Test initialization and POST request
TEST_F(HTTPTest, Post) {
    TestHTTPPostConnection conn;

    auto st = conn.Init();
    ASSERT_TRUE(st.ok()) << st.message();

    st = conn.Execute([](const web::http::http_response &response) {
        auto json = response.extract_json().get();

        EXPECT_EQ("POST", json.at("method").as_string());
        EXPECT_EQ("/test", json.at("path").as_string());
        EXPECT_EQ("bar", json.at("body").at("foo").as_string());

        return absl::OkStatus();
    });
    ASSERT_TRUE(st.ok()) << st.message();
}

// Test initialization and request to non-existant URL
TEST_F(HTTPTest, NotFound) {
    TestHTTPNotFoundConnection conn;

    auto st = conn.Init();
    ASSERT_TRUE(st.ok()) << st.message();

    st = conn.Execute([](const web::http::http_response &response) {
        EXPECT_EQ(web::http::status_codes::NotFound, response.status_code());

        return absl::NotFoundError("not found");
    });
    ASSERT_FALSE(st.ok()) << st.message();
}
