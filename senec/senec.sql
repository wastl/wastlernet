CREATE EXTENSION IF NOT EXISTS timescaledb;

CREATE SEQUENCE IF NOT EXISTS seq_senec;

DROP TABLE IF EXISTS senec;
DROP TABLE IF EXISTS senec_mppt;
DROP TABLE IF EXISTS senec_ac;

CREATE TABLE senec (
                       time TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
                       id INT NOT NULL,
                       hausverbrauch NUMERIC(6,1) NULL,
                       pv_leistung NUMERIC(6,1) NULL,
                       netz_leistung NUMERIC(6,1) NULL,
                       batterie_leistung NUMERIC(6,1) NULL,
                       quellen_einspeisung NUMERIC(6,1) NULL,
                       quellen_bezug NUMERIC(6,1) NULL,
                       quellen_laden NUMERIC(6,1) NULL,
                       quellen_entladen NUMERIC(6,1) NULL,
                       batterie_temperatur NUMERIC(4,1) NULL,
                       batterie_soc NUMERIC(4,1) NULL,
                       batterie_spannung NUMERIC(4,1) NULL,
                       gesamt_strom NUMERIC(12,1) NULL,
                       gesamt_einspeisung NUMERIC(12,1) NULL,
                       gesamt_bezug NUMERIC(12,1) NULL,
                       gesamt_verbrauch NUMERIC(12,1) NULL,
                       gesamt_produktion NUMERIC(12,1) NULL,
                       system_pv_begrenzung INT NULL,
                       system_ac_leistung NUMERIC(6,1) NULL,
                       system_frequenz NUMERIC(3,1) NULL,
                       system_status INT NULL,
                       system_betriebsstunden INT NULL,
                       system_anzahl_batterien INT NULL,
                       system_gehaeuse_temperatur NUMERIC(4,1) NULL,
                       system_mcu_temperatur NUMERIC(4,1) NULL,
                       system_fan_speed NUMERIC(5,1) NULL
);
CREATE INDEX senec_id_idx ON senec(id);

CREATE TABLE senec_mppt (
                            senec_id INT NOT NULL,
                            mppt_id INT NOT NULL,
                            strom NUMERIC(4,1) NULL,
                            spannung NUMERIC(4,1) NULL,
                            leistung NUMERIC(4,1) NULL
);
CREATE UNIQUE INDEX senec_mppt_idx ON senec_mppt(senec_id, mppt_id);

CREATE TABLE senec_ac (
                          senec_id INT NOT NULL,
                          ac_id INT NOT NULL,
                          strom NUMERIC(4,1) NULL,
                          spannung NUMERIC(4,1) NULL,
                          leistung NUMERIC(6,1) NULL
);
CREATE UNIQUE INDEX senec_ac_idx ON senec_ac(senec_id, ac_id);

SELECT create_hypertable('senec','time');

GRANT INSERT ON senec TO wastlernet;
GRANT INSERT ON senec_mppt TO wastlernet;
GRANT INSERT ON senec_ac TO wastlernet;
GRANT ALL ON seq_senec TO wastlernet;