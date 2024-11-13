#ifndef _Neptune_Protocol_h_
#define _Neptune_Protocol_h_

#include <Arduino.h>
//#include <String>

class NeptuneProtocol {
  int clock_pin;
  int read_pin;
  int relay_pin;
  bool read_pin_pullup;
  int buff[50];

  unsigned long initBlip = 302;  // target 304      
  unsigned long powerUP = 54971; // 54910  54971??;
  unsigned long blip = 422; // 423 from pulseview;
  unsigned long iamhere = 2451; // 2451;
//  unsigned long ClkTime = 425; // 431; 425 works for ARB meters
  unsigned long ClkTime = 425; // 431; 421=844 422=846 423=848   425=852 

  byte clkState = LOW;
  int readVar = 0;
  unsigned long readCurrentMicros = 0;
  unsigned long readPreviousClkMicros = 0;

//These are reversed due to inverting by NPN
// Clock ON should mean that the voltage to the meter is HIGH.

  static const auto clock_ON = LOW;
  static const auto clock_OFF = HIGH;
//  static const auto clock_ON = HIGH;
//  static const auto clock_OFF = LOW;
 
  void powerUp();
  void powerDown();
  int readBit();
  char readByte();
 /* struct reading
  {
	  String swver;
	  String readVal;
	  String serialNum;
	  String unknown1;
	  String unknown2;
	  String unknown3;
  };
  */
public:
  struct reading
  {
	  String swver;
	  String readVal;
	  String serialNum;
	  String unknown1;
	  String unknown2;
	  String unknown3;
  };
  NeptuneProtocol(int clock_pin, int read_pin, int relay_pin, bool read_pin_pullup = true);
  void setup(int reset_wait=5000);
  int getClockPin() const;
  void readData(int *buff, int max_bytes=50);
  void readMeter(reading *meterRead);
  void slowBitRead(int wait);
};


#endif //  _Neptune_Protocol_h_
