CREATE
EXTENSION IF NOT EXISTS timescaledb;

DROP TABLE IF EXISTS shelly_temperature;
DROP TABLE IF EXISTS shelly_light;
DROP TABLE IF EXISTS shelly_energy;

CREATE TABLE shelly_temperature
(
    time        TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
    device      VARCHAR(32) NOT NULL,
    temperature NUMERIC(3, 1) NULL,
    humidity    NUMERIC(4, 1) NULL
);
SELECT create_hypertable('shelly_temperature', 'time');

CREATE TABLE shelly_light
(
    time         TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
    device       VARCHAR(32) NOT NULL,
    lux          INTEGER NULL,
    illumination VARCHAR(10) NULL
);
SELECT create_hypertable('shelly_light', 'time');

CREATE TABLE shelly_energy
(
    time      TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
    device    VARCHAR(32) NOT NULL,
    power     NUMERIC(6, 2),
    voltage   NUMERIC(5, 2),
    current   NUMERIC(5, 3),
    frequency NUMERIC(4, 1)
);
SELECT create_hypertable('shelly_energy', 'time');
