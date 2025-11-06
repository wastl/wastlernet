//
// Created by wastl on 07.04.23.
//

#include "senec_timescaledb.h"

absl::Status senec::SenecWriter::prepare(pqxx::connection &conn) {
    conn.prepare("senec_insert_main",R"(
INSERT INTO senec (
   id,
   hausverbrauch,
   pv_leistung,
   netz_leistung,
   batterie_leistung,
   quellen_einspeisung,
   quellen_bezug,
   quellen_laden,
   quellen_entladen,
   batterie_temperatur,
   batterie_soc,
   batterie_spannung,
   gesamt_strom,
   gesamt_einspeisung,
   gesamt_bezug,
   gesamt_verbrauch,
   gesamt_produktion,
   system_pv_begrenzung,
   system_ac_leistung,
   system_frequenz,
   system_status,
   system_betriebsstunden,
   system_anzahl_batterien,
   system_gehaeuse_temperatur,
   system_mcu_temperatur,
   system_fan_speed
) VALUES (
   nextval('seq_senec'),$1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12,$13,$14,$15,$16,$17,$18,$19,$20,$21,$22,$23,$24,$25
)
)");

    conn.prepare("senec_insert_mppt",R"(
INSERT INTO senec_mppt (
   senec_id,
   mppt_id,
   strom,
   spannung,
   leistung
) VALUES (
   currval('seq_senec'), $1, $2, $3, $4
)
)");

    conn.prepare("senec_insert_ac",R"(
INSERT INTO senec_ac (
   senec_id,
   ac_id,
   strom,
   spannung,
   leistung
) VALUES (
   currval('seq_senec'), $1, $2, $3, $4
)
)");
    return absl::OkStatus();
}

absl::Status senec::SenecWriter::write(pqxx::work &tx, const senec::SenecData &data) {
    tx.exec(
        pqxx::prepped{"senec_insert_main"},
        pqxx::params{
            data.leistung().hausverbrauch(),
            data.leistung().pv_leistung(),
            data.leistung().netz_leistung(),
            data.leistung().batterie_leistung(),
            data.quellen().einspeisung(),
            data.quellen().bezug(),
            data.quellen().laden(),
            data.quellen().entladen(),
            data.batterie().temperatur(),
            data.batterie().soc(),
            data.batterie().spannung(),
            data.gesamt().strom(),
            data.gesamt().einspeisung(),
            data.gesamt().bezug(),
            data.gesamt().verbrauch(),
            data.gesamt().produktion(),
            data.system().pv_begrenzung(),
            data.system().ac_leistung(),
            data.system().frequenz(),
            data.system().status(),
            data.system().betriebsstunden(),
            data.system().anzahl_batterien(),
            data.system().gehaeuse_temperatur(),
            data.system().mcu_temperatur(),
            data.system().fan_speed()
        }
    );

    for (int i=0; i<data.mppt_size(); i++) {
        tx.exec(
            pqxx::prepped{"senec_insert_mppt"},
            pqxx::params{
                i,
                data.mppt(i).strom(),
                data.mppt(i).spannung(),
                data.mppt(i).leistung()
            }
        );
    }

    for (int i=0; i<data.mppt_size(); i++) {
        tx.exec(
            pqxx::prepped{"senec_insert_ac"},
            pqxx::params{
                i,
                data.ac_data(i).strom(),
                data.ac_data(i).spannung(),
                data.ac_data(i).leistung()
            }
        );
    }

    return absl::OkStatus();
}
