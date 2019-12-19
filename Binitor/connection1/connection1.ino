#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <HCSR04.h>
#include "DHT.h" // Temp. library
#include <ArduinoJson.h>


#define DHTPIN 13
#define DHTTYPE DHT11 
DHT dht(DHTPIN, DHTTYPE); 
int tiltPin = 5; 
const double BIN_HEIGHT = 17.0;
UltraSonicDistanceSensor distanceSensor(0, 4);  // NodeMCU pins: D3, D2
int ReportingInterval = 10; 

// Watson IoT connection details
#define MQTT_HOST "a2zn3f.messaging.internetofthings.ibmcloud.com"
#define MQTT_PORT 1883
#define MQTT_DEVICEID "d:a2zn3f:ESP8266:dev01"
#define MQTT_USER "use-token-auth"
#define MQTT_TOKEN "token123"
#define MQTT_TOPIC "iot-2/evt/status/fmt/json"
#define MQTT_TOPIC_DISPLAY "iot-2/cmd/display/fmt/json"
#define MQTT_TOPIC_INTERVAL "iot-2/cmd/interval/fmt/json"

// MQTT objects
void callback(char* topic, byte* payload, unsigned int length);
WiFiClient wifiClient;
PubSubClient mqtt(MQTT_HOST, MQTT_PORT, callback, wifiClient);

// JSON objects
StaticJsonDocument<100> jsonDoc;
JsonObject payload = jsonDoc.to<JsonObject>();
StaticJsonDocument<100> jsonReceiveDoc;
static char msg[100];

void callback(char* topic, byte* payload, unsigned int length) 
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] : ");

  payload[length] = 0; // ensure valid content is zero terminated so can treat as c-string

  DeserializationError err = deserializeJson(jsonReceiveDoc, (char *)payload);
  
  if (err) {
    Serial.print(F("deserializeJson() failed with code ")); 
    Serial.println(err.c_str());

    return;
  }

  if(0 == strcmp(topic, MQTT_TOPIC_INTERVAL)) 
  {
    JsonObject cmdData = jsonReceiveDoc.as<JsonObject>();
    ReportingInterval = cmdData["Interval"].as<int>();
    
    Serial.print("Reporting Interval has been changed:");
    Serial.println(ReportingInterval);
    jsonReceiveDoc.clear();    
  }
}

void setup() {
  startSerialConsole();
  Serial.println();
  Serial.println("Binitor");

  connectToWifi();
  connectMqttToIoTPlatform();
  
  pinMode(tiltPin, INPUT);  // set the tilt switch pin as input
  dht.begin();
}

void loop() {
  mqtt.loop();
  
  while (!mqtt.connected()) 
  {
    delay(5000);
    connectMqttToIoTPlatform();
  }

  float tiltState = digitalRead(tiltPin);
  double distance = distanceSensor.measureDistanceCm();
  Serial.print("Distance: ");
  Serial.println(distance);
  
  if(distance > BIN_HEIGHT) 
  {
    return;
  }
 
  double garbageLevel = getGarbageLevel(distance);

  float tempCelsius = dht.readTemperature();

  payload["garbageLevel"] = garbageLevel;
  payload["temperature"] = tempCelsius;
  payload["orientation"] = tiltState;
  serializeJson(jsonDoc, msg, 100);

  Serial.println(msg);

  if (!mqtt.publish(MQTT_TOPIC, msg)) 
  {
    Serial.println("MQTT Publish failed");
  }

  Serial.print("Sleep Time: ");
  Serial.print(getSleepTime(garbageLevel));
  pauseWhilePollingMqtt(getSleepTime(garbageLevel));
}

void startSerialConsole() 
{
  Serial.begin(115200);
  Serial.setTimeout(2000);
  while (!Serial) { }
}


void connectToWifi() 
{
  char ssid[] = "oi_eita_hotspot";
  char pass[] = "rocksana";
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("WiFi Connected");
}

void connectMqttToIoTPlatform() 
{  
  Serial.print("Attempting MQTT connection...");
  
  if (mqtt.connect(MQTT_DEVICEID, MQTT_USER, MQTT_TOKEN)) 
  {
    Serial.println("MQTT Connected");
    mqtt.subscribe(MQTT_TOPIC_DISPLAY);
    mqtt.subscribe(MQTT_TOPIC_INTERVAL);
    mqtt.loop();
  } 
  else 
  {
    Serial.println("MQTT Failed to connect!");
    ESP.reset();
  }
}

void pauseWhilePollingMqtt(int sec) 
{
  for (int i = 0; i < sec; i++) 
  {
    mqtt.loop();
    delay(1000);
  }  
}

int getGarbageLevel(double distance) 
{  
  double num = distance/BIN_HEIGHT;
  int level =  100 - ((num) * 100);

  if(level < 0) 
  {
    return 0;
  }
  else if (level > 100) 
  {
    return 100;
  }
  else 
  {
    return level;    
  }

}

int getSleepTime(double garbageLevel) 
{  
  if(ReportingInterval == -1) 
  {
    if(garbageLevel < 50) 
    {
      return 30; 
    }
    else if (garbageLevel < 80) 
    {
      return 20;
    }
    else 
    {
      return 10; 
    }
  } else 
  {
    return ReportingInterval;
  }
}
