//
// Created by wastl on 05.04.23.
//

#include "solvis_timescaledb.h"

absl::Status solvis::SolvisWriter::prepare(pqxx::connection &conn) {
    conn.prepare("solvis_insert", R"(
INSERT INTO solvis(
    speicher_oben,
    heizungspuffer_oben,
    heizungspuffer_unten,
    speicher_unten,
    warmwasser,
    kaltwasser,
    zirkulation,
    durchfluss,
    solar_kollektor,
    solar_vorlauf,
    solar_ruecklauf,
    solar_waermetauscher,
    solar_leistung,
    vorlauf_heizkreis1,
    vorlauf_heizkreis2,
    vorlauf_heizkreis3,
    kessel,
    kessel_leistung,
    kessel_ladepumpe,
    kessel_brenner,
    pumpe_heizkreis1,
    pumpe_heizkreis2,
    pumpe_heizkreis3
) VALUES ($1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12,$13,$14,$15,$16,$17,$18,$19,$20,$21,$22,$23))"
        );
    return absl::OkStatus();
}

absl::Status solvis::SolvisWriter::write(pqxx::work &tx, const solvis::SolvisData &data) {
    tx.exec(
        pqxx::prepped{"solvis_insert"},
        pqxx::params{
            data.speicher_oben(),
            data.heizungspuffer_oben(),
            data.heizungspuffer_unten(),
            data.speicher_unten(),
            data.warmwasser(),
            data.kaltwasser(),
            data.zirkulation(),
            data.durchfluss(),
            data.solar_kollektor(),
            data.solar_vorlauf(),
            data.solar_ruecklauf(),
            data.solar_waermetauscher(),
            data.solar_leistung(),
            data.vorlauf_heizkreis1(),
            data.vorlauf_heizkreis2(),
            data.vorlauf_heizkreis3(),
            data.kessel(),
            data.kessel_leistung(),
            data.kessel_ladepumpe(),
            data.kessel_brenner(),
            data.pumpe_heizkreis1(),
            data.pumpe_heizkreis2(),
            data.pumpe_heizkreis3()
        }
    );
    return absl::OkStatus();
}
