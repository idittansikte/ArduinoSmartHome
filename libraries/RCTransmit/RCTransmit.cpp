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
  
  if(mProtocol == 2)
    {
      this->setPulseLength(260);
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

void RCTransmit::switchOn(int controller, byte protocol, bool group, int device, byte buttonCode)
{
  start(controller, protocol, true, group, device, buttonCode);
}

void RCTransmit::switchOff(int controller, byte protocol, bool group, int device, byte buttonCode)
{
  start(controller, protocol, false, group, device, buttonCode);
}

void RCTransmit::start(int controller, byte protocol, bool status, bool group, int device, byte buttonCode)
{
  char* buffer = new char[40];
  this->mProtocol = protocol;
  if(this->mProtocol == 1)
    {
      this->setRepeatTransmit(6);
      this->getCodeProtocol1(buffer, controller, group, status, device);
      this->send(buffer);
    }
  else if(protocol == 2)
    {
      this->setRepeatTransmit(7);
      this->getCodeProtocol2(buffer, controller, 0/*group*/, status, 3/*device*/, (byte)3/*buttonCode*/);
      this->send(buffer);
    }
  delete[] buffer;
}
void RCTransmit::getCodeProtocol1(char* &buffer, const int &controller, const bool &group, const bool &status,const int &device)
{
  byte controllerBits = 26;
  byte deviceBits = 4;
  //char result[33];
  unsigned int resultPos = 0;

  // Get bits of controller
  this->insert( buffer, resultPos, controller, controllerBits );
  // Set group flag
  if(group)
    {
      insert( buffer, resultPos, 1, 1 );
    }
  else
    {
      insert( buffer, resultPos, 0, 1 );
    }
  // Set status
  if(status)
    {
      insert( buffer, resultPos, 1, 1 );
    }
  else
    {
      insert( buffer, resultPos, 0, 1 );
    }
  // Set device
  insert( buffer, resultPos, device, deviceBits );
  buffer[resultPos] = '\0';
}

void RCTransmit::insert(char*& buffer, unsigned int& pos, const int& input, const byte& numofbits){

  byte endpos = pos + numofbits;
  int i = 0;
  for(; pos < endpos; ++i)
    {
      int mask = 1 << i;
      int masked_n = input & mask;
      int thebit = masked_n >> i;
      buffer[pos++] = (char) thebit + 48;
    }
  buffer[pos] = '\0';
}

void RCTransmit::getCodeProtocol2(char* &buffer, const int &controller, const bool &group, 
				  const bool &status,const int &channel, const byte &buttonCode)
{

  byte controllerBits = 26; 
  byte deviceBits = 2;
  //char result[33];
  unsigned int resultPos = 0;

  // Get bits of controller
  insert( buffer, resultPos, controller, controllerBits );
  // Set group flag
  if(group)
    {
      insert( buffer, resultPos, (byte)1, (byte)1 );
    }
  else
    {
      insert( buffer, resultPos, (byte)0, (byte)1 );
    }
  // Set status
  if(status)
    {
      insert( buffer, resultPos, (byte)1, (byte)1 );
    }
  else
    {
      insert( buffer, resultPos, (byte)0, (byte)1 );
    }
  // Set device
  insert( buffer, resultPos, 3/*channel*/, deviceBits );

  // Set buttonCode
  insert( buffer, resultPos, 3/*buttonCode*/, deviceBits );

  buffer[resultPos] = '\0';
}

void RCTransmit::send(const char* code)
{
  Serial.print("Sending code:");
  Serial.println(code);
  for(int repeat = 0; repeat < mRepeatTransmit; ++repeat)
    {

      this->sendSync();

      // Send startbit if protocol 2
      //if(this->mProtocol == 2)
      //	this->transmit(P2_0_TIMING_HIGH, P2_0_TIMING_LOW);

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
      // PAUSE
      // if(this->mProtocol == 2){
      //	this->transmit(P2_0_TIMING_HIGH, P2_0_TIMING_LOW);
      //}
      this->transmit(P2_PAUSE_HIGH, P2_PAUSE_LOW);
	// else
	//	delay(10);

    } // Repeat-loop
}


void RCTransmit::sendSync()
{
  if(this->mProtocol == 1)
    {
      this->transmit(P1_SYNC_HIGH, P1_SYNC_LOW);
    }
  else if(this->mProtocol == 2)
    {
      this->transmit(P2_SYNC_HIGH, P2_SYNC_LOW);
    }
}

/**
 * Sends data "0"
 *     
 * Protocol 1&2:  _    _
 *     Waveform: | |__| |_____
 *     Bits: 01
 */
void RCTransmit::send0()
{
  if(this->mProtocol == 1)
    {
      this->transmit(P1_0_TIMING_HIGH, P1_0_TIMING_LOW);
      this->transmit(P1_1_TIMING_HIGH, P1_1_TIMING_LOW);
    }
  else if (mProtocol == 2)
    {
      this->transmit(P2_0_TIMING_HIGH, P2_0_TIMING_LOW);
      this->transmit(P2_1_TIMING_HIGH, P2_1_TIMING_LOW);
    }
}

/**
 * Sends data "1"
 *     
 * Protocol 1&2:  _       _
 *     Waveform: | |_____| |__
 *     Bits: 10
 */
void RCTransmit::send1()
{
  if(this->mProtocol == 1)
    {
      this->transmit(P1_1_TIMING_HIGH, P1_1_TIMING_LOW);
      this->transmit(P1_0_TIMING_HIGH, P1_0_TIMING_LOW);
    }
  else if (mProtocol == 2)
    {
      this->transmit(P2_1_TIMING_HIGH, P2_1_TIMING_LOW);
      this->transmit(P2_0_TIMING_HIGH, P2_0_TIMING_LOW);
    }
}

void RCTransmit::transmit(const int &highPulses, const int &lowPulses)
{
  digitalWrite(this->mTransmitPin, HIGH);
  delayMicroseconds(highPulses);
  digitalWrite(this->mTransmitPin, LOW);
  delayMicroseconds(lowPulses);
}

