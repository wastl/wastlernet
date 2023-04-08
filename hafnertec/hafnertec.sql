CREATE EXTENSION IF NOT EXISTS timescaledb;

DROP TABLE IF EXISTS hafnertec;
CREATE TABLE hafnertec (
    time TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
    temp_brennkammer NUMERIC(5,1) NULL,
    temp_ruecklauf NUMERIC(4,1) NULL,
    temp_vorlauf NUMERIC(4,1) NULL,
    durchlauf INT NULL,
    ventilator INT NULL,
    anteil_heizung INT NULL
);

SELECT create_hypertable('hafnertec','time');