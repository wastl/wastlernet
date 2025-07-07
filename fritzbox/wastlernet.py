import sys

from fritzconnection import FritzConnection
from fritzconnection.lib.fritzhomeauto import FritzHomeAutomation

if len(sys.argv) != 4:
    print("Invalid number of arguments")
    sys.exit(1)

fc = FritzConnection(address=sys.argv[1], user=sys.argv[2], password=sys.argv[3])
fh = FritzHomeAutomation(fc)

room_temp_sensors = {
    "13979 0435236": "Stube EG",
    "09995 0604665": "Stube OG",
    "08761 0090704": "Büro",
    "13979 0916516": "Besprechungszimmer",
    "09995 0604676": "Schlafzimmer",
}

devices = [d for d in fh.get_homeautomation_devices() if d.is_temperature_sensor]

for d in devices:
    #print("Found Device: %s (type %s, name %s)\n" % (d.AIN, d.ProductName, d.DeviceName))
    if d.AIN in room_temp_sensors:
        print("Temperatur %s: %.1f" % (room_temp_sensors[d.AIN], d.TemperatureCelsius*0.1))