//
// Created by wastl on 07.04.23.
//
#pragma once
#include "hafnertec_client.h"
#include "base/module.h"
#include "hafnertec/hafnertec.pb.h"
#include "hafnertec/hafnertec_timescaledb.h"

#ifndef WASTLERNET_HAFNERTEC_MODULE_H
#define WASTLERNET_HAFNERTEC_MODULE_H
namespace hafnertec {
    class HafnertecModule : public wastlernet::PollingModule<HafnertecData> {
    private:
        HafnertecClient client_;

    protected:
        absl::Status Query(std::function<absl::Status(const HafnertecData &)> handler) override;

    public:
        HafnertecModule(const wastlernet::TimescaleDB& db_cfg, const wastlernet::Hafnertec& client_cfg, wastlernet::StateCache* c)
        : PollingModule(db_cfg, new HafnertecWriter, c, client_cfg.poll_interval()),
                client_(client_cfg.host(), client_cfg.user(), client_cfg.password()) {}

        std::string Name() override {
            return "hafnertec";
        }

        absl::Status Init() override;
    };
};
#endif //WASTLERNET_HAFNERTEC_MODULE_H
