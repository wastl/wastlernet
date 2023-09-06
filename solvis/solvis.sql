CREATE EXTENSION IF NOT EXISTS timescaledb;

DROP TABLE IF EXISTS solvis;
CREATE TABLE solvis (
    time TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
    speicher_oben NUMERIC(4,1) NULL,
    heizungspuffer_oben NUMERIC(4,1) NULL,
    heizungspuffer_unten NUMERIC(4,1) NULL,
    speicher_unten NUMERIC(4,1) NULL,
    warmwasser NUMERIC(3,1) NULL,
    kaltwasser NUMERIC(3,1) NULL,
    zirkulation NUMERIC(5,1) NULL,
    durchfluss NUMERIC(5,1) NULL,
    solar_kollektor NUMERIC(4,1) NULL,
    solar_vorlauf NUMERIC(4,1) NULL,
    solar_ruecklauf NUMERIC(4,1) NULL,
    solar_waermetauscher NUMERIC(4,1) NULL,
    solar_leistung NUMERIC(4,2) NULL,
    vorlauf_heizkreis1 NUMERIC(4,1) NULL,
    vorlauf_heizkreis2 NUMERIC(4,1) NULL,
    vorlauf_heizkreis3 NUMERIC(4,1) NULL,
    pumpe_heizkreis1 BOOLEAN NULL,
    pumpe_heizkreis2 BOOLEAN NULL,
    pumpe_heizkreis3 BOOLEAN NULL,
    kessel NUMERIC(4,1) NULL.
    kessel_leistung NUMERIC(4,1) NULL,
    kessel_ladepumpe BOOLEAN NULL,
    kessel_brenner BOOLEAN NULL
);

SELECT create_hypertable('solvis','time');