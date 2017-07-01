MQTT Raw Data to Cayenne Dashboard,
using the great Nick O'Leary's PubSubClient
(Arduino Client for MQTT, https://github.com/knolleary/pubsubclient)

What does it do?
This sketch will attempt to connect to Cayenne MQTT server via Wifi,
subscribe to a "dashboard button" topic, to turn on/off the esp8266 built in led.
Also it will publish temperature readings from a Dallas DS18b20 digital temperature sensor.

The esp8266 (NodeMCU v2) should act as a smart thermostat, controlling my central heater/boiler.
It should be possible to remotely check ambient temperature and boiler status using
Cayenne Dashboard and turn the heater on/off (in case I left home in a hurry!, etc.)


Inspired in several tutorials and sketches mashed up, mainly..
Andreas Spiess  https://github.com/SensorsIot
Terry King      terry@yourduino.com
Markus Ulsass   https://github.com/markbeee

Marc / spacefolder
