CREATE EXTENSION IF NOT EXISTS timescaledb;

CREATE SEQUENCE IF NOT EXISTS seq_hue_sensors;
CREATE SEQUENCE IF NOT EXISTS seq_hue_lights;

DROP TABLE IF EXISTS hue_sensors;
DROP TABLE IF EXISTS hue_lights;

CREATE TABLE hue_sensors (
                       time TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
                       id INT NOT NULL,
                       uid VARCHAR(64) NOT NULL,
                       name VARCHAR(128) NOT NULL,
                       lightLevel NUMERIC(5,0) NULL,
                       daylight BOOLEAN NULL,
                       dark BOOLEAN NULL,
                       presence BOOLEAN NULL,
                       reachable BOOLEAN NULL,
                       temperature NUMERIC(2,1) NULL
);

CREATE INDEX hue_sensors_name_idx ON hue_sensors(name);

SELECT create_hypertable('hue_sensors','time');