/*********
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com
  Tutorial link: https://randomnerdtutorials.com/esp32-esp8266-plot-chart-web-server/

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  For the NTP stuff this tutorial was followed:
  http://suadica.com/dica.php?d=439&t=como-utilizar-relogio-rtc-interno-do-esp32
*********/

// Import required libraries
#include <WiFi.h>
#include <WiFiUDP.h>
#include <NTPClient.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
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

const float TEMPERATURE_MIN = 30.0;
const float TEMPERATURE_MAX = 40.0;

WiFiUDP udp;
NTPClient ntp(udp, "a.st1.ntp.br", -3 * 3600, 60000);//Creates a NTP object with BRT timezone
struct tm date; //date strutcture
int turnOnTimestamp;
char formatted_date[64];

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

void setupRTCTime(){
  Serial.println("Setting RTC time...");
  turnOnTimestamp = ntp.getEpochTime(); //Get time from NTP
  Serial.println("Timestamp from NTP: ");
  Serial.print(int(time));
  timeval tv; //timeval structure
  tv.tv_sec = turnOnTimestamp;  //configure RTC with timestamp
  settimeofday(&tv, NULL); //configure RTTC to maintain correct time
  time_t tt = time(NULL); // get time now
  date = *gmtime(&tt); //convert time_t to tm struct
  strftime(formatted_date, 64, "%d/%m/%Y %H:%M:%S", &date); //convert tm struct to string
  Serial.println('Date updated: ');
  Serial.print(String(formatted_date));
}

String readFormattedDate(){
  time_t tt = time(NULL);
  date = *gmtime(&tt); //convert time_t to tm struct
  strftime(formatted_date, 64, "%d/%m/%Y %H:%M:%S", &date); //convert tm struct to string
  return String(formatted_date);
}

String readTurnOnTimestamp(){
  return String(turnOnTimestamp);
}


void controlHeater( void * parameter )
{
  float    temperature = 100.0f;
  float    humidity = 100.0f;
  bool     heated = false;
  int      TimeToControlHeater = 2000 * 1000; // 5 seconds
  int      TimeForAM2302reading = 5000 * 1000; // 2000mS or 2 seconds
  int      printCounts = 0;
  uint64_t TimePastControlHeater = esp_timer_get_time(); // used to control heater
  uint64_t TimePastAM2302reading   = esp_timer_get_time();
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = 100; //delay for 100mS
  for (;;)
  {
    //read AM2302 values every TimeForAM2302reading
    if ( (esp_timer_get_time() - TimePastAM2302reading) >= TimeForAM2302reading )
    {
      DHTSensor.read();
      humidity = DHTSensor.getHumidity();
      temperature = DHTSensor.getTemperature();
      Serial.println("Past 5 seconds, reading sensor values");
      Serial.print("Temperature: ");
      Serial.println(temperature);
      Serial.print("Humidity: ");
      Serial.println(humidity);
      Serial.println();

      TimePastAM2302reading = esp_timer_get_time();
    }

    if( (esp_timer_get_time() - TimePastControlHeater) >= TimeToControlHeater )
    {
      Serial.println("Past 2 seconds, controlling heater");
      Serial.println();

      if(!heated && temperature < TEMPERATURE_MAX)
      {
        //turn on heater
        digitalWrite(17, HIGH);
        digitalWrite(18, HIGH);
        Serial.println("Heater ON");
        Serial.println();
      }

      if(heated && temperature < TEMPERATURE_MIN)
      {
        //turn on heater
        digitalWrite(17, HIGH);
        digitalWrite(18, HIGH);
        Serial.println("Heater ON");
        Serial.println();
        heated = false;
      }

      if(temperature > TEMPERATURE_MAX)
      {
        //turn off heater
        digitalWrite(17, LOW);
        digitalWrite(18, LOW);
        Serial.println("Heater OFF");
        Serial.println();
        heated = true;
      }

      TimePastControlHeater = esp_timer_get_time();
    }

    xLastWakeTime = xTaskGetTickCount();
    vTaskDelayUntil( &xLastWakeTime, xFrequency );
  }
  vTaskDelete( NULL );
}// end controlHeater()


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
  server.on("/date", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readFormattedDate().c_str());
  });
  server.on("/turnon", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readTurnOnTimestamp().c_str());
  });

  // Start server
  server.begin();

  ntp.begin(); //Start NTP
  ntp.forceUpdate(); //Update time for the first time
  setupRTCTime(); //Setup RTC time

  xTaskCreate(
    controlHeater,   // Function that should be called
    "Control Heater",// Name of the task (for debugging)
    1000,            // Stack size (bytes)
    NULL,            // Parameter to pass
    1,               // Task priority
    NULL             // Task handle
  );
}
void loop(){
}
