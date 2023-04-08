//
// Created by wastl on 07.04.23.
//

#include "hafnertec_timescaledb.h"

absl::Status hafnertec::HafnertecWriter::prepare(pqxx::connection &conn) {
    conn.prepare("hafnertec_insert", R"(
INSERT INTO hafnertec (
    temp_brennkammer,
    temp_ruecklauf,
    temp_vorlauf,
    durchlauf,
    ventilator,
    anteil_heizung
) VALUES ($1,$2,$3,$4,$5,$6))"
    );
    return absl::OkStatus();
}

absl::Status hafnertec::HafnertecWriter::write(pqxx::work &tx, const HafnertecData &data) {
    tx.exec_prepared("hafnertec_insert",
                     data.temp_brennkammer(),
                     data.temp_ruecklauf(),
                     data.temp_vorlauf(),
                     data.durchlauf(),
                     data.ventilator(),
                     data.anteil_heizung()
    );
    return absl::OkStatus();
}
