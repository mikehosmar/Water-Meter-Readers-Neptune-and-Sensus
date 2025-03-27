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

// #define USE_CLASS

#ifdef USE_CLASS
#include <NeptuneProtocol.h>
int  meter1_clock_pin = 2;   // black
int  meter1_read_pin  = 3;  // red
int  meter1_relay_pin = 4;
bool pullup           = true;

NeptuneProtocol          meter1(meter1_clock_pin, meter1_read_pin, meter1_relay_pin, pullup);
NeptuneProtocol::reading meter1reading;

#else

#include <SPI.h>
// Code straight from Bob Prust
#define RxData 3
#define TxClock 2
#define Relay 4


// SPIClass * spi = NULL;
// const int spiClk = 12000;


/////////////////////////////Read Meter declarations
unsigned int dataAlign[35];

// 35 is ok Buffer for bit read data

unsigned int meterByte[35];

// 35 is ok

int count = 9;

// byte timing tuning

int bitcount = 0;
int mask = 15;

// mask 0b0000 0000 0000 1111. Strips 4 bit integer

unsigned int last = 0;
unsigned int last_A = 0;

byte set_P = 0;
byte set = 0;

// Send Data record count
// Send Data count register

// Send Data start / stop flag
// command flag in ethernet read

byte command = 0; // Data command

unsigned long previousMillis = 0; // Delay between Meter reads

int bitRate = 420;

// 1187hz. Seems more stable than 1200

boolean state = false;

boolean laststate = false;

#endif
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

#ifdef USE_CLASS
  meter1.setup();
#else
  pinMode(RxData, INPUT_PULLUP); // Read
  pinMode(TxClock, OUTPUT);      // clock
  pinMode(Relay, OUTPUT);
  // spi = new SPIClass(HSPI);
  // spi->begin(TxClock,RxData, 5, 6); // HSPI

#endif

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

#ifndef USE_CLASS
void MeterRead() {
  digitalWrite(TxClock, LOW); // set up to put an initial low on clk line
  digitalWrite(Relay, HIGH);
  delay(200);

  // Clk until Rx line changes (Up to 10 minutes)
  state = digitalRead(RxData);
  while (state == digitalRead(RxData)) {
    digitalWrite(TxClock, LOW); 
    delayMicroseconds(bitRate);
    if (state != digitalRead(RxData))
      break;
    digitalWrite(TxClock, HIGH);
    delayMicroseconds(bitRate);
  }

  // Look for Rx line to go Low (62 - 95mS)

  // Quickly align transistion of state change
  state = digitalRead(RxData);
  while (state == digitalRead(RxData)) {
    for (int y = 0; y < 32; y++) {
      if (y == 0)
        digitalWrite(TxClock, LOW);
      if (y == 15)
        digitalWrite(TxClock, HIGH);
      delayMicroseconds(20);
      if (state != digitalRead(RxData))
        break;
    }
  }

  // Read 34 Data bytes. 316mS to 319mS per 34 bytes read.

  for (int mData = 0; mData < 34; mData++) {
    for (int bytecount = 0; bytecount < 11;
         bytecount++) { // 11 bits per byte incl. 2 stop and 1 start
      for (bitcount = 7; bitcount >= 0; bitcount--) {

        // read each bit 8 times. 4 high, 4 low

        if (bitcount == 7)
          digitalWrite(TxClock, LOW);
        if (bitcount == 3)
          digitalWrite(TxClock, HIGH);
        delayMicroseconds(96);

        // 1180 bits/Sec. 107 bytes/Sec

        laststate = !digitalRead(RxData);
        if (bitcount == 5)
          bitWrite(meterByte[mData], bytecount, laststate); // write bit state
      }

      delayMicroseconds(count); // fine tune timing. Count from 7 to 11
    }
    // dataAlign[mData] = meterByte[mData]; //align 11 bit bytes
    if (bitRead(meterByte[mData], 10) == 0)
      dataAlign[mData] = meterByte[mData]; // should be start bit
    if (bitRead(meterByte[mData], 10) > 0)
      dataAlign[mData] = meterByte[mData] >> 1; // shift right may correct align
    if ((bitRead(meterByte[mData], 5) > 0) &&
                      (bitRead(meterByte[mData], 6) > 0)) dataAlign[mData] =
        meterByte[mData] >> 1; // bit 5 & 6 should be 1
    if ((bitRead(meterByte[mData], 6) > 0) &&
        (bitRead(meterByte[mData], 7) > 0))
      dataAlign[mData] = meterByte[mData] >> 2; // align 11 bit bytes

    meterByte[mData] = dataAlign[mData] &
                       mask; // meterData is least 4 bits of masked aligned data

    // delayMicroseconds(count); // maybe more fine timing tuning
  }

  // exit read loop

  digitalWrite(TxClock, LOW); // put low on meter

  digitalWrite(Relay, LOW); // Turn off relay before writing to SD card
}
// end data capture
#endif

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
#ifdef USE_CLASS
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
#else
    MeterRead();
#endif
  }

  WebSerial.loop();
  ArduinoOTA.handle();
}