#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "wifi_credentials.h"

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WebSerial.h>
// #include <MycilaWebSerial.h>

AsyncWebServer server(80);

#define DEBUG_ESP_PORT WebSerial
#include <NeptuneProtocol.h>
int  meter1_clock_pin = SCK;   // black
int  meter1_read_pin  = MISO;  // red
int  meter1_relay_pin = 1;
bool pullup           = false;

NeptuneProtocol          meter1(meter1_clock_pin, meter1_read_pin, meter1_relay_pin, pullup);
NeptuneProtocol::reading meter1reading;

void setup()
{
  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.setHostname("water-meter");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  ArduinoOTA
      .onStart([]() {
        String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
        Serial.println("Start updating " + type);
      })
      .onEnd([]() { Serial.println("\nEnd"); })
      .onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
      })
      .onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR)
          Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR)
          Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR)
          Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR)
          Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR)
          Serial.println("End Failed");
      });

  ArduinoOTA.begin();

  server.onNotFound([](AsyncWebServerRequest* request) { request->redirect("/webserial"); });
  WebSerial.begin(&server);
  server.begin();

  meter1.setup();

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

unsigned long last_print_time = millis();
unsigned long last_read_time;

void loop()
{
  // Print every 30 seconds (non-blocking)
  if ((unsigned long)(millis() - last_print_time) > 30000)
  {
    WebSerial.print(F("IP address: "));
    WebSerial.println(WiFi.localIP());
    WebSerial.printf("Uptime: %lums\n", millis());
    WebSerial.printf("Free heap: %" PRIu32 "\n", ESP.getFreeHeap());
    last_print_time = millis();
  }

  // Read meter every 10 seconds
  if ((unsigned long)(millis() - last_read_time) > 10000)
  {
    last_read_time = millis();
    meter1reading  = {};  // Clear struct
    meter1.readMeter(&meter1reading);
    WebSerial.println("Meter 1");
    WebSerial.print(" Serial Number : ");
    WebSerial.println(meter1reading.serialNum);
    WebSerial.print(" Reading Value : ");
    WebSerial.println(meter1reading.readVal);
    WebSerial.print(" Software Ver  : ");
    WebSerial.println(meter1reading.swver);
    WebSerial.print(" Unknown 1     : ");
    WebSerial.println(meter1reading.unknown1);
    WebSerial.print(" Unknown 2     : ");
    WebSerial.println(meter1reading.unknown2);
    WebSerial.print(" Unknown 3     : ");
    WebSerial.println(meter1reading.unknown3);
  }

  WebSerial.loop();
  ArduinoOTA.handle();
}