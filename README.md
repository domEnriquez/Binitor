# Binitor
Smart Garbage Monitoring System

The Arduino code publishes the gathers data from the ultrasonic, tilt, and temperature sensors and sends them to the IBM IoT platform through the MQTT protocol.

## Setup function
The setup function has the following responsibilities:
1. Connects the ESP8266 to a WiFi
   - The ESP8266WiFi library is used and credentials are hardcoded in the code
2. Connects the ESP8266 to the IoT Platform
   - All the credentials and configuration details for connecting to the IBM IoT platform are hardcoded at the top of the code
3. Initializes all the sensors
   - Sets the pins ESP8266 pins used by the sensors
   - Calls the initialize functions of the third-party sensor libraries used

## Loop function
The loop function has following role for every iteration:
1. Listen for messages from the IoT platform through the mqtt.loop() function
2. Gather data from the sensors
3. Send sensor data to the IoT platform through the mqtt.publish() function
4. Controls the pause time per iteration depending on the interval data received from the IoT platform

## Other Public/Private Functions
The other functions are short and well-named so they can be intuitively understood by any reader of the code.

## Third Party Libraries
- ArduinoJson - provides functions for data to JSON format
- PubSubClient - provides functions for sending/receivng data to/from the IoT platfrom through MQTT protocol
- HCSR04 - provides functions for getting the measured distance of the ultrasonic sensor
- DHT - provides functions for getting the measured temperature of the temperature sensor
