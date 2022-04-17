/*********
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com
  Tutorial link: https://randomnerdtutorials.com/esp32-esp8266-plot-chart-web-server/

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*********/

// Import required libraries
#ifdef ESP32
  #include <WiFi.h>
  #include <ESPAsyncWebServer.h>
  #include <SPIFFS.h>
#else
  #include <Arduino.h>
  #include <ESP8266WiFi.h>
  #include <Hash.h>
  #include <ESPAsyncTCP.h>
  #include <ESPAsyncWebServer.h>
  #include <FS.h>
#endif

#include <dhtnew.h>

// DHT PIN layout from left to right
// =================================
// FRONT : DESCRIPTION
// pin 1 : VCC
// pin 2 : DATA
// pin 3 : Not Connected
// pin 4 : GND

DHTNEW DHTSensor(5);   // ESP 16    UNO 5    MKR1010 5

// Replace with your network credentials
const char* ssid = "greenhouse";
const char* password = "senhasupersecreta";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

String readAM2302Temperature() {
  DHTSensor.read();
  float t = DHTSensor.getTemperature();
  if (isnan(t)) {
    Serial.println("Failed to read from AM2302 sensor!");
    return "";
  }
  else {
    Serial.println(t);
    return String(t);
  }
}

String readAM2302Humidity() {
  DHTSensor.read();
  float h = DHTSensor.getHumidity();
  if (isnan(h)) {
    Serial.println("Failed to read from AM2302 sensor!");
    return "";
  }
  else {
    Serial.println(h);
    return String(h);
  }
}

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);

  // Initialize SPIFFS
  if(!SPIFFS.begin()){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // Print ESP32 Local IP Address
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html");
  });
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readAM2302Temperature().c_str());
  });
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readAM2302Humidity().c_str());
  });

  // Start server
  server.begin();
}

void loop(){

}
