#ifndef RCTransmit_H
#define RCTransmit_H

#include "Arduino.h"

/**
 *
 * #### Protocol 1 ####
 *
 * Tested on Proove Remote Switches
 * 
 * Starts with sync: high for DELAY_SHORT (260) us and low for DELAY_START (2675) us
 * 
 * Coding:
 * 0 = DELAY_SHORT high and DELAY_SHORT low
 * 1 = DELAY_SHORT high and DELAY_LONG low
 *
 * Data 0 = 01
 * Data 1 = 10
 *
 * The actual message is 32 bits of data (64 wire bits):
 * bits 0-25: the group code - a 26bit number assigned to controllers.
 * bit 26: group flag
 * bit 27: on/off flag
 * bits 28-31: the device code - a 4bit number.
 */
#define P1_1_TIMING_HIGH 260
#define P1_1_TIMING_LOW 1300
/*  _
 * | |_____
 */
#define P1_0_TIMING_HIGH 260
#define P1_0_TIMING_LOW 260
/*  _
 * | |_
 */

#define P1_SYNC_HIGH 260
#define P1_SYNC_LOW 2600
/*   _
 *  | |__________
 */
// #### END OF PROTOCOL 1 ####

/**
 *
 * #### Protocol 2 ####
 *
 * Tested on Nexa Remote Switches 
 * 
 * 
 * Timing:
 *   "1" = 295us high and 170us low
 *   "0" = 295us high and 920us low
 * 
 * Coding:
 *   Data 0 = 01
 *   Data 1 = 10
 *
 * Beginning:
 *   Sync, high 295us and a low 2500us
 * 
 * Bit    DataNr   Meaning
 * 01     1        Startbit
 * 02-53  2-27     Unique 26-bit sender code.
 * 54-55  28       Group on/off
 * 56-57  29       10=On 01=Off 11=Dim
 * 58-61  30-31    Channel code
 * 62-65  32-33    Button code
 * 66-73  34-37    Dimmer grade, 16 stages
 * 
 * Channel/button code:
 * 1010 = 1
 * 1001 = 2
 * 0110 = 3
 * 0101 = 4
 * 
 * End:
 *   Low 295us
 * 
 * Time between sendings:
 *   10ms
 */


#define P2_0_TIMING_HIGH 235
#define P2_0_TIMING_LOW 280
#define P2_1_TIMING_HIGH 235
#define P2_1_TIMING_LOW 1333
#define P2_SYNC_HIGH 235
#define P2_SYNC_LOW 2700
#define P2_PAUSE_HIGH 235
#define P2_PAUSE_LOW 10000
// #### END OF PROTOCOL 2 ####

class RCTransmit 
{
public:
  RCTransmit();
  RCTransmit(int transmitPin);

  void setProtocol(int protocol);
  void setPulseLength(int pulseLenght);
  void setRepeatTransmit(int repeat);
  //void switchOn(int controller, bool group, int device);
  //void switchOff(int controller, bool group, int device);
  void switchOn(int controller, byte protocol, bool group, int device, byte buttonCode = 0);
  void switchOff(int controller, byte protocol, bool group, int device, byte buttonCode = 0);

private:
  void start(int controller, byte protocol, bool status, bool group, int device, byte buttonCode);

  void insert(char*& buffer, unsigned int& pos, const int& input, const byte& numofbits);
  //TODO: Change name when more protocols with same sending sequence are implemented. 
  void getCodeProtocol1(char* &buffer, const int &controller, const bool &group, const bool &status, const int &device);
  void getCodeProtocol2(char* &buffer, const int &controller, const bool &group, 
			const bool &status,const int &channel, const byte &buttonCode);
  //TODO: Change name when more protocols with same sending sequence are implemented. 
  void send(const char* code);

  void sendSync();
  void send0();
  void send1();
  void transmit(const int &highPulses, const int &lowPulses);

  int mProtocol;
  const int mTransmitPin;
  int mPulseLength;
  int mRepeatTransmit;
};

#endif
