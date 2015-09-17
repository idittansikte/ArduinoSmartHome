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


#define PROTOCOL1_SYNC_HIGH_CYCLES 1
#define PROTOCOL1_SYNC_LOW_CYCLES 10
/*   _
 *  | |__________
 */
#define PROTOCOL1_0_HIGH_CYCLES 1
#define PROTOCOL1_0_LOW_CYCLES 1
/*  _
 * | |_
 */
#define PROTOCOL1_1_HIGH_CYCLES 1
#define PROTOCOL1_1_LOW_CYCLES 5
/*  _
 * | |_____
 */
// #### END OF PROTOCOL 1 ####

class RCTransmit 
{
public:
  RCTransmit() = delete;
  RCTransmit(int transmitPin);

  void setProtocol(int protocol);
  void setPulseLength(int pulseLenght);
  void setRepeatTransmit(int repeat);
  void switchOn(int controller, bool group, int device);
  void switchOff(int controller, bool group, int device);
  

private:

  //TODO: Change name when more protocols with same sending sequence are implemented. 
  void getCodeProtocol1(char* &buffer, const int &controller, const bool &group, const bool &status, const int &device);
  //TODO: Change name when more protocols with same sending sequence are implemented. 
  void sendProtocol1(const char* code);

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
