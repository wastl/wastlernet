CREATE
EXTENSION IF NOT EXISTS timescaledb;

DROP TABLE IF EXISTS shelly_temperature;
DROP TABLE IF EXISTS shelly_light;

CREATE TABLE shelly_temperature
(
    time        TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
    device		VARCHAR(32) NOT NULL,
    temperature NUMERIC(3, 1) NULL,
    humidity    NUMERIC(4, 1) NULL
);
SELECT create_hypertable('shelly_temperature', 'time');

CREATE TABLE shelly_light
(
    time        TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
    device		VARCHAR(32) NOT NULL,
    lux         INTEGER NULL,
    illumination VARCHAR(10) NULL
);
SELECT create_hypertable('shelly_light', 'time');
