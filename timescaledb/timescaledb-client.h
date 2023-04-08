//
// Created by wastl on 03.04.23.
//
#include <absl/status/status.h>
#include <absl/strings/str_format.h>
#include <google/protobuf/message.h>
#include <glog/logging.h>
#include <pqxx/pqxx>

#ifndef WASTLERNET_TIMESCALEDB_CLIENT_H
#define WASTLERNET_TIMESCALEDB_CLIENT_H
namespace timescaledb {
    template<class Data>
    class TimescaleWriter {
    public:
        /*
         * Prepare a statement for later use.
         */
        virtual absl::Status prepare(pqxx::connection& conn) = 0;

        /*
         * Write a piece of data to the database using the transaction handed over as first argument. The transaction
         * will be committed on OK status or rolled back otherwise.
         */
        virtual absl::Status write(pqxx::work& tx, const Data& data) = 0;
    };

    template<class Data>
    class TimescaleConnection {
    private:
        std::string host_, db_, user_, password_;
        int port_;

        std::unique_ptr<pqxx::connection> conn_;
        std::unique_ptr<TimescaleWriter<Data>> writer_;
    public:
        // Takes ownership of writer
        TimescaleConnection(TimescaleWriter<Data>* writer, absl::string_view db, absl::string_view host, int port, absl::string_view user, absl::string_view password)
                : writer_(writer), db_(db), host_(host), port_(port), user_(user), password_(password) { }

        absl::Status Init();

        absl::Status Update(const Data& data) {
            pqxx::work tx(*conn_);
            auto st = writer_->write(tx, data);
            if (st.ok()) {
                tx.commit();
            } else {
                tx.abort();
            }
            return st;
        }

        absl::Status Close();
    };

}

template<class Data>
absl::Status timescaledb::TimescaleConnection<Data>::Close() {
    try {
        conn_->close();
        return absl::OkStatus();
    } catch (std::exception const &e) {
        LOG(ERROR) << "Database error on close: " << e.what();
        return absl::InternalError(e.what());
    }
}

template<class Data>
absl::Status timescaledb::TimescaleConnection<Data>::Init() {
    try {
        std::string conn_str = absl::StrFormat("postgresql://%s:%s@%s:%d/%s", user_, password_, host_, port_, db_);
        conn_ = std::make_unique<pqxx::connection>(conn_str);
        return writer_->prepare(*conn_);
    } catch (std::exception const &e) {
        LOG(ERROR) << "Database error initialising connection: " << e.what();
        return absl::InternalError(e.what());
    }
}
#endif //WASTLERNET_TIMESCALEDB_CLIENT_H
