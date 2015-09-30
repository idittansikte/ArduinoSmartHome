#ifndef _NTP_REAL_TIME_
#define _NTP_REAL_TIME_

#include "Arduino.h"
#include <SPI.h>
//#include <Ethernet.h>
#include <EthernetUdp.h>

// IPAddress timeServer(132, 163, 4, 101); // time-a.timefreq.bldrdoc.gov NTP server
// IPAddress timeServer(132, 163, 4, 102); // time-b.timefreq.bldrdoc.gov NTP server
// IPAddress timeServer(132, 163, 4, 103); // time-c.timefreq.bldrdoc.gov NTP server

typedef struct  { 
  uint8_t Second; 
  uint8_t Minute; 
  uint8_t Hour; 
  uint8_t Wday;   // day of week, sunday is day 1
  uint8_t Day;
  uint8_t Month; 
  uint8_t Year;   // offset from 1970; 
} tmElements_t;

typedef unsigned long time_t;

// NTP time stamp is in the first 48 bytes of the message
const int NTP_PACKET_SIZE=48;

class NTPRealTime {
 public:
  
  NTPRealTime();

  void init(IPAddress timeserver, int port);
  void setSyncInterval(int seconds);
  void setTimezone(int8_t timezone);
  void summertime(bool summertime);

  uint8_t getHour();
  uint8_t getMin();
  uint8_t getSec();

  time_t now();
 private:
  bool fetchNTPTime();
  time_t sendNTPpacket(IPAddress& address);

  void breakTime(time_t timeInput);
  void refreshCache(time_t t);

  EthernetUDP mUdp;
  IPAddress mTimeServer;
  time_t mUnixTime;
  unsigned int mSyncInterval; // In second
  time_t mLastSync;

  tmElements_t tm;
  time_t cacheTime;
  int8_t mTimezone;
  bool mSummertime;
};


#endif
