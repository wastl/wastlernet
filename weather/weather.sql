CREATE EXTENSION IF NOT EXISTS timescaledb;

DROP TABLE IF EXISTS weather;
CREATE TABLE weather (
    time TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
    uv NUMERIC(4,1) NULL,
    barometer NUMERIC(5,1) NULL,
    daily_rain NUMERIC(4,1) NULL,
    dewpoint NUMERIC(3,1) NULL,
    outdoor_temperature NUMERIC(3,1) NULL,
    outdoor_humidity NUMERIC(4,1) NULL,
    indoor_temperature NUMERIC(3,1) NULL,
    indoor_humidity NUMERIC(4,1) NULL,
    wind_direction INT NULL,
    wind_speed NUMERIC(3,1) NULL,
    wind_gusts NUMERIC(3,1) NULL,
    rain NUMERIC(4,1) NULL,
    solarradiation NUMERIC(5,1) NULL
);

SELECT create_hypertable('weather','time');
GRANT INSERT, SELECT ON weather TO wastlernet;