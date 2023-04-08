# SENEC Exporter

This is a simple tool written in C++ scraping a SENEC Home photovoltaic installation for 
performance data and writes it to an InfluxDB time series database for further analysis 
(e.g. in Grafana).

It's at the moment not very well tested and only works for me, but I'm offering the code 
in case someone else is interested in doing the same.