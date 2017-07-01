/*
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

*/

// ---------- INLCUDE LIBRARIES ----------

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h>              //http://www.pjrc.com/teensy/td_libs_OneWire.html
#include <DallasTemperature.h>


// ---------- DEFINE CONSTANT VALUES ----------

#define PIN_ONE_WIRE_BUS  2     // GPIO02, D4 on esp8266. Sensor's data input pin.

#define MQTT_SERVER     "mqtt.mydevices.com"
#define MQTT_SERVERPORT 1883
#define MQTT_USERNAME   "fxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxe"
#define MQTT_PASSWORD   "fxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxd"
#define CLIENT_ID       "7xxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxx0"

// Define "topic" linked to the Cayenne Dashboard ON/OFF button (boiler) (Cayenne Channel 5)
#define MQTT_TOPIC_BTN_ONOFF    "v1/" MQTT_USERNAME "/things/" CLIENT_ID "/cmd/5"
// Define "topic" linked to the Cayenne Dashboard Temperature Gauge (Cayenne Channel 10)
#define MQTT_TOPIC_TPROBE_INT   "v1/" MQTT_USERNAME "/things/" CLIENT_ID "/data/10"


// ---------- INITIALIZE INSTANCES ----------

OneWire oneWire(PIN_ONE_WIRE_BUS);    // Setup a oneWire instance to communicate with any OneWire devices
DallasTemperature sensors(&oneWire);  // Pass our oneWire reference to Dallas Temperature.

WiFiClient    espClient;              // Creates a client instance for the PubSubClient
PubSubClient  client(espClient);


// ---------- DEFINE VARIABLES ----------

const   char* ssid        = "xxxxxxxxxxx"; // WLAN-SSID (WiFi router)
const   char* password    = "xxxxxxxxx"; // WLAN Password

// Probe01 is the Hex Adress of the Dallas Temperature sensor. In the future, more than one 
// sensor will be used, readings are done using the hardware address of each one.
DeviceAddress Probe01 = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // Probe #001

// Temperature readings will be once every 2 seconds. Define variables to use with "millis" function.
long    timeStampNow;
long    timeStampLastMsg = 0;

// Define a datatype to store the mqtt message, up to 50 bytes.
char    msg[50];                 // 50 BYTES QUIZAS SEA MUCHO AL GAS, pero bueh

// This variable will store the temperature readings from the Dallas probe.
float   tempDallasProbe1;




// -------------------- SETUP -------------------

void setup() {

  // Initialize serial monitor for debugging purposes.
  Serial.begin(9600);
  Serial.println("DEBUG: Entering setup ");

  // Attempt to connect to WiFi.
  WiFi.begin(ssid, password);
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");                       // Faltaria agregar que muestre el ip si no es estatico
  Serial.println("WiFi connected");

  // Initialize Dallas temperature library/
  Serial.print("Initializing Temperature Control Library Version ");
  Serial.println(DALLASTEMPLIBVERSION);
  sensors.begin();
  // set the DS18b20's resolution to 10 bit (0.25 C degress)
  sensors.setResolution(Probe01, 10);
  
  // Check if temperature sensors are present. 
  Serial.println();
  Serial.print("DEBUG: Number of devices found on the OneWire bus = ");
  Serial.println(sensors.getDeviceCount());
  Serial.println();


  // Initialize Pins
  pinMode(BUILTIN_LED, OUTPUT);       // esp8266's built in led.
  digitalWrite(BUILTIN_LED, !LOW);    // This pin handles inverse logic, it's state is reversed.


  // Initialize MQTT PubSub Library
  client.setServer(MQTT_SERVER, MQTT_SERVERPORT);
  // Function called in case of an incomming mqtt message from Cayenne Dashboard
  // (In this case is the dashboard button to turn on/off the heater)
  client.setCallback(mqttCallback);

  Serial.println("DEBUG: Setup Done! ");

}




// -------------------- LOOOP -------------------

void loop() {

  Serial.println("DEBUG: Entering main loop ");

  // Check if the esp8266 is connected to the mqtt server.
  if (!client.connected()) {                            
    reconnect();                                        
  }

  // If connected, perform PubSubClient housekeeping function.
  client.loop();                                        

  // Publish temperature readings every 2 seconds (2000ms)
  timeStampNow = millis();
  if (timeStampNow - timeStampLastMsg > 2000) {    // Publish interval in milliseconds
    // Reset the counter
    timeStampLastMsg = timeStampNow;
    // Jump to the actual function to read and publish temperatures.
    dealWithTemperatures();
  }

  delay(10);
  Serial.println("DEBUG: Reached MAIN LOOP END --> Looping ");
}







// ------------------------------ OTHER FUNCTIONS

// Function that attempts to reconnect to the mqtt server.

void reconnect() {                                  
  // If esp8266 is disconnected from the mqtt server, try to reconnect.
  // (*** I still need to think what wil happen to the boiler if connection is lost ***)
  while (!client.connected()) {
    Serial.print("DEBUG: Attempting MQTT connection...");

    // Attempt to connect
    if (client.connect(CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
      Serial.println("connected");
      
      // Once connected, resubscribe to the Dashboard on/off button topic.
      client.subscribe(MQTT_TOPIC_BTN_ONOFF);

    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
  Serial.println("DEBUG: Quiting reconnect loop ");
}




// Function to intercept MQTT messages

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    // Show the received message "stream" in the serial monitor.
    Serial.print((char)payload[i]);
  }
  Serial.println();


  // The actual Cayenne mqtt message payload (Actuator Command) consists of:
  // a Sequence Identifier followed by the actual Value (0 = Off, 1 = On)
  // For example: "2otoExGxnMJz0Jn,0". We need the Value that is in "position" number 16.
  if ((char)payload[16] == '0') {

    // We received a 0, so turn built in led off.
    digitalWrite(BUILTIN_LED, !LOW);
    // Publish confirmation that the led was actually turned off, so it is reflected in the dashboard.
    client.publish("v1/" MQTT_USERNAME "/things/" CLIENT_ID "/data/5", "0");
  }

  if ((char)payload[16] == '1') {

    // We received a 1, so turn built in led on.
    digitalWrite(BUILTIN_LED, !HIGH);
    // Publish confirmation that the led was actually turned off, so it is reflected in the dashboard.
    client.publish("v1/" MQTT_USERNAME "/things/" CLIENT_ID "/data/5", "1");
  }

  //  Delay for debugging purposes. If a message arrives from Cayenne, the terminal feed pauses 5'.
  delay(5000);
  
}


// Function that handles temperature adquisition from sensors, and publishing to Cayenne Dashboard.

void dealWithTemperatures() {
  
  Serial.print(" Requesting temperatures...");
  sensors.requestTemperatures(); // Send the command to get temperatures of all devices on the bus.
  Serial.println("DONE");

  // Get the temperature value of sensor with hex address Probe01
  tempDallasProbe1 = sensors.getTempC(Probe01);

  // Construct the mqtt message following Cayenne's rules, according to the Docs..
  // Send Sensor data -> Topic: v1/username/things/clientID/data/channel  
  // (*** Still need to optimize the use of strings in this part!!! ***)
  String stringOne = "temp,c=";
  
  //using a float rounded to 2 decimal places.
  String stringTwo = String(tempDallasProbe1, 2);
  
  // Concatenate the two previous strings into mqtt_message.
  String mqtt_message =  String(stringOne + stringTwo);
  Serial.println(mqtt_message);


  // Convert the string into a Char Array
  mqtt_message.toCharArray(msg, 20);
  

  Serial.print("Publish message: ");
  Serial.println(msg);
  // Finally! Publish the temperature to Cayenne Dashboard, Channel 10.
  client.publish(MQTT_TOPIC_TPROBE_INT, msg);

  
}




