#include "RCTransmit.h"

RCTransmit::RCTransmit(int transmitPin)
  : mTransmitPin(transmitPin)
{
  this->setProtocol(1);
  this->setRepeatTransmit(6);
}

void RCTransmit::setProtocol(int protocol)
{
  this->mProtocol = protocol;
  
  if(mProtocol == 1)
    {
      this->setPulseLength(270);
    }
}

void RCTransmit::setPulseLength(int pulseLength)
{
  this->mPulseLength = pulseLength;
}

void RCTransmit::setRepeatTransmit(int repeat)
{
  this->mRepeatTransmit = repeat;
}

void RCTransmit::switchOn(int controller, bool group, int device)
{
  if(this->mProtocol == 1)
    {
      char* buffer = new char[33];
      this->getCodeProtocol1(buffer, controller, group, true, device);
      this->sendProtocol1(buffer);
      delete[] buffer;
    }
}

void RCTransmit::switchOff(int controller, bool group, int device)
{
  if(this->mProtocol == 1)
    {
      char* buffer = new char[33];
      this->getCodeProtocol1(buffer, controller, group, false, device);
      this->sendProtocol1(buffer);
      delete[] buffer;
    }
}

void RCTransmit::getCodeProtocol1(char* &result, const int &controller, const bool &group, const bool &status,const int &device)
{
  unsigned int controllerBits = 26;
  unsigned int deviceBits = 4;
  //char result[33];
  int resultPos = 0;

  // Validate input
  if(controller < 10 || controller > 255 || device < 0 || device > 4)
    {
      result[0] = '\0';
	return;
    }
  // Get bits of controller
  for(unsigned i = 0; i < controllerBits; ++i)
    {
      int mask = 1 << i;
      int masked_n = controller & mask;
      int thebit = masked_n >> i;
      result[resultPos++] = (char) thebit + 48;
    }
  // Set group flag
  if(group)
    {
      result[resultPos++] = '1';
    }
  else
    {
      result[resultPos++] = '0';
    }
  // Set status
  if(status)
    {
      result[resultPos++] = '1';
    }
  else
    {
      result[resultPos++] = '0';
    }
  // Set device
  for(unsigned i = 0; i < deviceBits; ++i)
    {
      int mask = 1 << i;
      int masked_n = device & mask;
      int thebit = masked_n >> i;
      result[resultPos++] = (char) thebit + 48;
    }
   result[resultPos] = '\0';
}

void RCTransmit::sendProtocol1(const char* code)
{
  for(int repeat = 0; repeat < mRepeatTransmit; ++repeat)
    {
      this->sendSync();
      
      int i = 0;
      while (code[i] != '\0') 
	{
	  switch(code[i])
	    {
	    case '0':
	      this->send0();
	      break;
	    case '1':
	      this->send1();
	      break;
	    }
	  i++;
	} // Code-loop

    } // Repeat-loop
}


void RCTransmit::sendSync()
{
  if(this->mProtocol == 1)
    {
      this->transmit(PROTOCOL1_SYNC_HIGH_CYCLES, PROTOCOL1_SYNC_LOW_CYCLES);
    }
}

/**
 * Sends data "0"
 *     
 * Protocol 1:    _    _
 *     Waveform: | |__| |_____
 *     Bits: 01
 */
void RCTransmit::send0()
{
  if(this->mProtocol == 1)
    {
      this->transmit(PROTOCOL1_0_HIGH_CYCLES, PROTOCOL1_0_LOW_CYCLES);
      this->transmit(PROTOCOL1_1_HIGH_CYCLES, PROTOCOL1_1_LOW_CYCLES);
    }
}

/**
 * Sends data "1"
 *     
 * Protocol 1:    _       _
 *     Waveform: | |_____| |__
 *     Bits: 10
 */
void RCTransmit::send1()
{
  if(this->mProtocol == 1)
    {
      this->transmit(PROTOCOL1_1_HIGH_CYCLES, PROTOCOL1_1_LOW_CYCLES);
      this->transmit(PROTOCOL1_0_HIGH_CYCLES, PROTOCOL1_0_LOW_CYCLES);
    }
}

void RCTransmit::transmit(const int &highPulses, const int &lowPulses)
{
  digitalWrite(this->mTransmitPin, HIGH);
  delayMicroseconds( this->mPulseLength * highPulses);
  digitalWrite(this->mTransmitPin, LOW);
  delayMicroseconds( this->mPulseLength * lowPulses);
}

