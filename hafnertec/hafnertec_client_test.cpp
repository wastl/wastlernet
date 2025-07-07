//
// Created by wastl on 07.07.25.
//
#include <thread>
#include <gtest/gtest.h>

#include "hafnertec_client.h"

#include <absl/strings/strip.h>
#include <cpprest/http_listener.h>
#include <glog/logging.h>

using web::http::http_request;
using web::http::experimental::listener::http_listener;

constexpr char kAddress[] = "http://127.0.0.1:15003";
constexpr char kContent[] = R"(<div id="pos0" >
60.0 °C</div>
<div id="pos1" >
18.8°
</div>
<div id="pos2" class="changex2" adresse="10F44140A91">
90.0 %</div>
<div id="pos3" >
24.9°
</div>
<div id="pos4" >
24.8°
</div>
<div id="pos5" class="changex2" adresse="00840D10A91">
0.0%
</div>
<div id="pos6" class="changex2" adresse="03C48120A91">
-10.0°
</div>
<div id="pos7" class="changex2" adresse="10E44140A91">
80.0 °C</div>
<div id="pos8" >
T. Max VL AWE</div>
<div id="pos9" class="changex2" adresse="00940D10A91">
0.0%
</div>
<div id="pos10" class="durchsichtig" onClick="location.href='schema.html#1'">
</div>
<div id="pos11" class="durchsichtig" onClick="location.href='schema.html#3'">
</div>
<div id="pos12" class="durchsichtig" onClick="location.href='schema.html#1'">
</div>
<div id="pos13" class="durchsichtig" onClick="location.href='schema.html#2'">
</div>)";


class HafnertecClientTest : public testing::Test {
protected:
    HafnertecClientTest() {
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
                } else if (path == "schematic_files/9.cgi") {
                    request.reply(web::http::status_codes::OK, kContent, "text/html").get();
                } else {
                    // everything else should return NOT FOUND
                    request.reply(web::http::status_codes::NotFound,
                                  absl::StrCat("data for module \"", path, "\" not found"), "text/plain").get();
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

TEST_F(HafnertecClientTest, Query) {
    hafnertec::HafnertecClient client(kAddress, "user", "password");

    auto st = client.Init();
    ASSERT_TRUE(st.ok()) << "Could not initialize Hafnertec client: " << st;

    st = client.Query([](const hafnertec::HafnertecData& data) {
        EXPECT_DOUBLE_EQ(data.temp_brennkammer(), 18.8);
        EXPECT_DOUBLE_EQ(data.temp_vorlauf(), 24.9);
        EXPECT_DOUBLE_EQ(data.temp_ruecklauf(), 24.8);
        EXPECT_DOUBLE_EQ(data.durchlauf(), 0.0);
        EXPECT_DOUBLE_EQ(data.anteil_heizung(), 90.0);
    });
    ASSERT_TRUE(st.ok()) << "Querying Hafnertec data failed: " << st;
}