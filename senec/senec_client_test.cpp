//
// Created by wastl on 08.07.25.
//
#include <thread>
#include <gtest/gtest.h>

#include "senec_client.h"

#include <absl/strings/strip.h>
#include <cpprest/http_listener.h>
#include <glog/logging.h>

using web::http::http_request;
using web::http::experimental::listener::http_listener;

constexpr char kAddress[] = "http://127.0.0.1:15003";
constexpr char kContent[] = R"({
  "PV1": {
    "MPP_CUR": [
      "fl_3F716873",
      "fl_3F79DB24",
      "fl_00000000"
    ],
    "MPP_VOL": [
      "fl_43C9AF9E",
      "fl_440F33A6",
      "fl_00000000"
    ],
    "MPP_POWER": [
      "fl_43BE170B",
      "fl_440AED0F",
      "fl_00000000"
    ]
  },
  "PM1OBJ1": {
    "P_TOTAL": "fl_4274147B",
    "FREQ": "fl_424847AE",
    "U_AC": [
      "fl_436C0000",
      "fl_436CE667",
      "fl_436D199A"
    ],
    "I_AC": [
      "fl_40428F5C",
      "fl_402851EB",
      "fl_3F933333"
    ],
    "P_AC": [
      "fl_4403070A",
      "fl_C41A5E14",
      "fl_431A63D7"
    ]
  },
  "ENERGY": {
    "GUI_BAT_DATA_FUEL_CHARGE": "fl_00000000",
    "GUI_BAT_DATA_POWER": "fl_C0BF0220",
    "GUI_BAT_DATA_VOLTAGE": "fl_4237AD0E",
    "GUI_HOUSE_POW": "fl_447AB7E0",
    "GUI_GRID_POW": "fl_4274147B",
    "GUI_INVERTER_POWER": "fl_4469F894",
    "STAT_STATE": "u8_0F",
    "STAT_HOURS_OF_OPERATION": "u3_00009B5D"
  },
  "BMS": {
    "NR_INSTALLED": "u8_04",
    "TOTAL_CURRENT": "VARIABLE_NOT_FOUND"
  },
  "STATISTIC": {
    "LIVE_GRID_IMPORT": "VARIABLE_NOT_FOUND",
    "LIVE_GRID_EXPORT": "VARIABLE_NOT_FOUND",
    "LIVE_HOUSE_CONS": "VARIABLE_NOT_FOUND",
    "LIVE_PV_GEN": "VARIABLE_NOT_FOUND",
    "CURRENT_STATE": "VARIABLE_NOT_FOUND"
  },
  "TEMPMEASURE": {
    "BATTERY_TEMP": "fl_42040000",
    "CASE_TEMP": "fl_420B2845",
    "MCU_TEMP": "fl_424368B3"
  },
  "FAN_SPEED": {
    "INV_LV": "u8_00"
  }
})";

class SenecClientTest : public testing::Test {
protected:
    SenecClientTest() {
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
                } else if (path == "lala.cgi") {
                    auto body = request.extract_json().get();

                    // Test if request body is present
                    ASSERT_TRUE(body.has_object_field("PV1"));
                    ASSERT_TRUE(body.has_object_field("ENERGY"));

                    request.reply(web::http::status_codes::OK, kContent, "text/json").get();
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

TEST_F(SenecClientTest, Query) {
    senec::SenecClient client(kAddress);

    auto st = client.Init();
    ASSERT_TRUE(st.ok()) << "Could not initialize SENEC client: " << st;

    st = client.Query([](const senec::SenecData &data) {
        ASSERT_TRUE(data.has_leistung());
        EXPECT_GT(data.leistung().pv_leistung(), 900.0);
        EXPECT_GT(data.leistung().hausverbrauch(), 1000.0);
        EXPECT_NEAR(
            data.leistung().pv_leistung() + data.leistung().netz_leistung() - data.leistung().batterie_leistung(),
            data.leistung().hausverbrauch(), 0.001);
        data.PrintDebugString();
    });
    ASSERT_TRUE(st.ok()) << "Querying SENEC data failed: " << st;
}
