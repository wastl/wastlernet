//
// Created by wastl on 09.12.22.
//

#include <absl/strings/str_join.h>
#include <cctype>
#include <iostream>
#include <cpprest/uri.h>                        // URI library
#include <cpprest/http_msg.h>
#include <cpprest/http_client.h>
#include <cpprest/json.h>
#include <glog/logging.h>
#include <gumbo-query/Document.h>
#include <gumbo-query/Node.h>

#include "hafnertec_client.h"

using namespace utility;                    // Common utilities like string conversions
using namespace web;                        // Common features like URIs.
using namespace web::http;                  // Common HTTP functionality
using namespace web::http::client;          // HTTP client features
using namespace concurrency::streams;       // Asynchronous streams

#define LOGH(level) LOG(level) << "[hafnertec] "

namespace hafnertec {
    double parse_numeric(absl::string_view s) {
        int start, end;

        // find start of number
        for (start = 0; start < s.size(); start++) {
            if (std::isdigit(s[start]) && s[start] != '-' && s[start] != '.' && s[start] != '+') {
                break;
            }
        }

        // find end of number
        for (end = s.size() - 1; end > start; end--) {
            if (std::isdigit(s[end]) && s[end] != '-' && s[end] != '.' && s[end] != '+') {
                break;
            }
        }

        if (start == end) {
            LOGH(ERROR) << "Could not find number in '" << s << "'";
            return 0;
        }

        std::string n = std::string(s.substr(start, end));

        return std::atof(n.c_str());
    }

    absl::Status query(absl::string_view uri, absl::string_view user, absl::string_view password,
               const std::function<void(const HafnertecData &)> &handler) {
        // Create http_client to send the request.
        std::string user_(user);
        std::string password_(password);
        web::credentials credentials(user_, password_);
        http_client_config config;
        config.set_credentials(credentials);

        http_client client(U(std::string(uri)), config);

        // request data
        LOGH(INFO) << "Querying Hafnertec controller";

        // Build request URI and start the request.
        uri_builder builder(U("/schematic_files/9.cgi"));
        try {
            return client.request(methods::GET, builder.to_string()).then(
                    [=](const http_response& response) {
                        if (response.status_code() == status_codes::OK) {

                            std::string html = absl::StrCat("<html>", response.extract_string().get(), "</html>");
                            CDocument doc;
                            doc.parse(html);

                            CSelection c = doc.find("html div");

                            hafnertec::HafnertecData data;
                            data.set_temp_brennkammer(parse_numeric(c.nodeAt(1).text()));
                            data.set_anteil_heizung(parse_numeric(c.nodeAt(2).text()));
                            data.set_temp_vorlauf(parse_numeric(c.nodeAt(3).text()));
                            data.set_temp_ruecklauf(parse_numeric(c.nodeAt(4).text()));
                            data.set_durchlauf(parse_numeric(c.nodeAt(5).text()));
                            data.set_ventilator(parse_numeric(c.nodeAt(9).text()));

                            LOGH(INFO) << "Received data from Hafnertec controller (chamber temperature "  << parse_numeric(c.nodeAt(1).text()) << ")";
                            LOGH(INFO) << "running handler";

                            handler(data);

                            return absl::OkStatus();
                        } else {
                            LOGH(ERROR) << "Hafnertec controller query failed: " << response.reason_phrase();
                            return absl::InternalError(absl::StrCat("Hafnertec controller query failed: ", response.reason_phrase()));
                        }
                    }).get();
        } catch (const std::exception &e) {
            LOGH(ERROR) << "Error while retrieving Hafnertec data: " << e.what();
            return absl::InternalError(absl::StrCat("Error while retrieving Hafnertec data: ", e.what()));
        }
    }


}  // namespace hafnertec