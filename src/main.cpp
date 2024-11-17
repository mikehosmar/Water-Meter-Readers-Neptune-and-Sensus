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
unsigned long  last_print_time = millis();

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
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
        {
          type = "sketch";
        }
        else
        {  // U_SPIFFS
          type = "filesystem";
        }

        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        Serial.println("Start updating " + type);
      })
      .onEnd([]() { Serial.println("\nEnd"); })
      .onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
      })
      .onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR)
        {
          Serial.println("Auth Failed");
        }
        else if (error == OTA_BEGIN_ERROR)
        {
          Serial.println("Begin Failed");
        }
        else if (error == OTA_CONNECT_ERROR)
        {
          Serial.println("Connect Failed");
        }
        else if (error == OTA_RECEIVE_ERROR)
        {
          Serial.println("Receive Failed");
        }
        else if (error == OTA_END_ERROR)
        {
          Serial.println("End Failed");
        }
      });

  ArduinoOTA.begin();

  // WebSerial is accessible at "<IP Address>/webserial" in browser
  server.onNotFound([](AsyncWebServerRequest* request) { request->redirect("/webserial"); });
  WebSerial.begin(&server);

  server.begin();

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop()
{
  // Print every 2 seconds (non-blocking)
  if ((unsigned long)(millis() - last_print_time) > 30000)
  {
    WebSerial.print(F("IP address: "));
    WebSerial.println(WiFi.localIP());
    WebSerial.printf("Uptime: %lums\n", millis());
    WebSerial.printf("Free heap: %" PRIu32 "\n", ESP.getFreeHeap());
    last_print_time = millis();
  }

  WebSerial.loop();
  ArduinoOTA.handle();
}