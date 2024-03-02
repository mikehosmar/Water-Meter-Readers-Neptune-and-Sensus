#include "NeptuneProtocol.h"
//#include <String>

#ifdef DEBUG_ESP_PORT
#define DEBUG_MSG(...) DEBUG_ESP_PORT.printf( __VA_ARGS__ )
#else
#define DEBUG_MSG(...)
#endif

void NeptuneProtocol::powerUp() {
//  delay(3000);
//  delayMicroseconds(50);
  digitalWrite(relay_pin, HIGH); // send power to the npn meter
  digitalWrite(clock_pin, clock_ON); // power on meter
  clkState = HIGH;
  //delay(1000);
}

void NeptuneProtocol::powerDown() {
  digitalWrite(clock_pin, clock_OFF); // power off meter
  digitalWrite(relay_pin, LOW); // power off the meter

}

int NeptuneProtocol::readBit() {
  /*
  digitalWrite(clock_pin, clock_OFF);
  delay(1);
//  delayMicroseconds(450);
  digitalWrite(clock_pin, clock_ON);
  delay(1); // Seems to work even without it, but just for sure
//  delayMicroseconds(450);
  int val = digitalRead(read_pin); 
  DEBUG_MSG("bit: %i\n", val);
// */
// First time in a session the clock will be high.  Then we want to Turn it Low after the right time, wait, turn it high, read data and return.
// PRE: Clock MUST be high to enter this area.
   
  int val = -1; // Setting val, our returnvalue here to be -1.
         while(readVar < 2000){ 
            if (readCurrentMicros - readPreviousClkMicros > ClkTime){ // change state every 550uS - So we enter here if the output needs to be flipped.
               if (clkState==LOW){  // First time in this will be false. so we jump down below.
                  clkState = HIGH;
                  digitalWrite(clock_pin, clock_ON);
				  // Need to wait here. to read the outout.
//				  val = -2;
// Read the signal in the middle here.
				  val = digitalRead(read_pin);

               }
               else { //clkState was HIGH .. Ok. so now we are in the first iteration, set the clock low.
                  clkState = LOW;
//                  digitalWrite(clock_pin, clock_OFF);
//                  if (val == -2) { 
//					  val = digitalRead(read_pin);
//                      digitalWrite(clock_pin, clock_OFF);
//                      readPreviousClkMicros = readCurrentMicros; // set flip time.
//					  break;
//				  }
				  digitalWrite(clock_pin, clock_OFF);

              }      
              readVar++;
              if(readVar % 2000 == 0) 
                 yield();
              readPreviousClkMicros = readCurrentMicros; // set flip time.
            }
			if (val != -1) { // bail if we have read something.
				break;
			}
            readCurrentMicros = micros();
         }




 
 /*
  int val = -1;
  readCurrentMicros = micros();
   while(readVar < 10000){ 
      if (readCurrentMicros - readPreviousClkMicros > ClkTime){ // change state every 550uS
        if (clkState==LOW){
           clkState = HIGH;
           digitalWrite(clock_pin, clock_ON);
           val = digitalRead(read_pin); // Read right after turning on the clock.
        }
        else { //clkState was HIGH
           clkState = LOW;
	       digitalWrite(clock_pin, clock_OFF);
		   if (val != -1) {
  //      readPreviousClkMicros = readCurrentMicros; // NOT SURE....

			   break;
		   }
        }      
        readVar++;
        if(readVar % 2000 == 0) 
           yield();
        readPreviousClkMicros = readCurrentMicros;
      }
      readCurrentMicros = micros();
   }
  // */
   return val;
}

char NeptuneProtocol::readByte() {
  int maxDec = 200; // can wait for this many idle bits
  int bits[10];
  
  for (int i = 0; i < 10; ++i) {
    bits[i] = readBit();
    if ((i == 0) && (bits[i] == 1)) //If we have not hit a start bit and we are
                                    // high this means that we are reading an
									// idle bit so mozy along. Guessing that we
									// will see a max of 20 idle bits.
    {
      --i; //decrease the i here since we are at an idle bit.
      if (--maxDec <= 0) {
         return -1;  // we have hit over maxDec idle bits.
      }
    }
  }
  char result = 0;
  int startIdx = 1; // Doing this since the 0 bit is the start bit
  int numOfBits = 7;
  for (int b = 0; b < 7; ++b) {
    int i = b + startIdx;
    if (bits[i]) {
      result |= (1 << b);
    }
  }
  DEBUG_MSG("byte: %i, %c\n", result, result);
  return result;    
}
  
NeptuneProtocol::NeptuneProtocol(int clock_pin, int read_pin, int relay_pin, bool read_pin_pullup)
 : clock_pin(clock_pin), read_pin(read_pin), relay_pin(relay_pin), read_pin_pullup(read_pin_pullup) {}

void NeptuneProtocol::setup(int reset_wait) {
  DEBUG_MSG("setup using pins: clk: %i, read: %i, read_pullup: %i ...\n", clock_pin, read_pin, read_pin_pullup);
  pinMode(clock_pin, OUTPUT);
  digitalWrite(clock_pin, clock_OFF); // power off the meter
  pinMode(relay_pin, OUTPUT);
  digitalWrite(relay_pin, LOW); // power off the meter

  auto input = INPUT;
  if (read_pin_pullup) {
    input = INPUT_PULLUP;
  }
  pinMode(read_pin, input);
//  delay(reset_wait); // make sure that the meter is reset
}

int NeptuneProtocol::getClockPin() const { return clock_pin; }

void NeptuneProtocol::readData(int *readInto, int max_bytes) { 
//	     previousClkMicros = 0;
         readVar = 0;
         readCurrentMicros = micros();

  for (int i = 0; i < max_bytes; ++i) {
    readInto[i] = readByte();
	if ((readInto[i] == 3) || (readInto[i] == -1)) // 3 is end of text, -1 is error/timeout meter not responding.
	{
	   if (readInto[i] == -1)
		   readInto[0] = 0;
       break;
    }
  }
}

void NeptuneProtocol::readMeter(reading *meterRead) {
//   struct reading meterRead;	
   int meterData[50] = {0}; 
//   memset(meterData, 0, sizeof(meterData)); // create array of ints here max of 50.  
   powerUp();
/*   
//   delay(5000);

delay(55);


//   delayMicroseconds	(54900); //delay for 55 milliseconds
   digitalWrite(clock_pin, clock_OFF);
   delay(1);
//   delayMicroseconds(450);
   digitalWrite(clock_pin, clock_ON);  
//   delay(1); // now this is one wave.
//   delay(4); //hold it high for two waves

delay(3);

//   delayMicroseconds(450);
//	 delayMicroseconds(2000);
*/
   unsigned long currentMillis = millis();
   unsigned long currentMicros = micros();
   unsigned long previousMillis = 0; // For Delay between meter reads
   unsigned long previousClkMicros = 0; // For TxClock timing
   readPreviousClkMicros = 0;
   readVar = 0;

   previousMillis = currentMillis;
   currentMicros = micros();
   previousClkMicros = currentMicros;
   
   digitalWrite(clock_pin,clock_OFF);
   while(currentMicros - previousClkMicros < initBlip) {
      yield();
      currentMicros = micros();
   }  // Now have powered on the meter for period of time.
   clkState = HIGH;
   digitalWrite(clock_pin,clock_ON);// Power up meter.  Wait for 54900 micros
 //           pinMode (read_pin,INPUT_PULLUP); // Pull RX High            
   previousClkMicros = currentMicros;
   currentMicros = micros();
   while(currentMicros - previousClkMicros < powerUP) {
      yield();
      currentMicros = micros();
   }  // Now have powered on the meter for period of time.
   previousClkMicros = currentMicros;
            //Drop Meter power for 'blip'
   clkState = LOW;
            currentMicros = micros();
   digitalWrite(clock_pin,clock_OFF);
   while(currentMicros - previousClkMicros < blip) {
  //previousClkMicros = currentMicros;
      currentMicros = micros();
   }  // Now have dipped blipp.
   previousClkMicros = currentMicros;
//            Raise meter power for 'here'
   clkState = HIGH;
   digitalWrite(clock_pin,clock_ON);
   currentMicros = micros();
   while(currentMicros - previousClkMicros < iamhere) {
      yield();
      currentMicros = micros();
   }  // Now we had said ready to rx to the meter.
   previousClkMicros = currentMicros;
 
   readData(meterData, 50);
   if ((meterData[0] == 0)||(meterData[0]== -1) || (meterData[14] == 0xff)) { // Not sure about the oxFF but that was contained in the string in a error reading so catching it here... sigh.
      (*meterRead).readVal =   "error";	  
	  (*meterRead).unknown1 =  "error";
	  (*meterRead).unknown2 =  "error";
	  (*meterRead).unknown3 =  "error";
	  (*meterRead).serialNum = "error";
  	  (*meterRead).swver =     "error";
   } else {
      (*meterRead).readVal += (char) meterData[7];
      (*meterRead).readVal += (char) meterData[8];
      (*meterRead).readVal += (char) meterData[9];
      (*meterRead).readVal += (char) meterData[10];
      (*meterRead).readVal += (char) meterData[11];
      (*meterRead).readVal += (char) meterData[12];
      if (meterData[27] != 32) { // 8 wheel meter
         (*meterRead).readVal += (char) meterData[27];
         (*meterRead).readVal += (char) meterData[28];
      }
	  (*meterRead).swver += (char) meterData[1];
	  (*meterRead).swver += (char) meterData[2];
 	  (*meterRead).swver += (char) meterData[3];
	  (*meterRead).swver += (char) meterData[4];
	  (*meterRead).swver += (char) meterData[5];
	  (*meterRead).unknown1 += (char) meterData[25];
	  (*meterRead).unknown2 += (char) meterData[29];
	  (*meterRead).unknown3 += (char) meterData[31];
	  (*meterRead).unknown3 += (char) meterData[32];
	  (*meterRead).serialNum += (char) meterData[14];
	  (*meterRead).serialNum += (char) meterData[15];
	  (*meterRead).serialNum += (char) meterData[16];
	  (*meterRead).serialNum += (char) meterData[17];
	  (*meterRead).serialNum += (char) meterData[18];
	  (*meterRead).serialNum += (char) meterData[19];
	  (*meterRead).serialNum += (char) meterData[20];
	  (*meterRead).serialNum += (char) meterData[21];
	  (*meterRead).serialNum += (char) meterData[22];
	  (*meterRead).serialNum += (char) meterData[23];
   }
   powerDown();
}



void NeptuneProtocol::slowBitRead(int wait) {
  powerUp();
  while (true) {
    readBit();
    delay(wait);
  }
}
