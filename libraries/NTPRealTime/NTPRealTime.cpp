
#include "NTPRealTime.h"

// leap year calulator expects year argument as years offset from 1970
#define LEAP_YEAR(Y)     ( ((1970+Y)>0) && !((1970+Y)%4) && ( ((1970+Y)%100) || !((1970+Y)%400) ) )

static  const uint8_t monthDays[]={31,28,31,30,31,30,31,31,30,31,30,31}; // API starts months from 1, this array starts from 0

NTPRealTime::NTPRealTime(){
  mLastSync=0;
  mTimezone=1;
  mSummertime = true;
  mSyncInterval=300;
}

void NTPRealTime::refreshCache(time_t t) {
  if (t != cacheTime) { // If more than one second passed
    breakTime(t); 
    cacheTime = t; 
  }
}

void NTPRealTime::setTimezone(int8_t timezone){
  mTimezone = timezone;
}

void NTPRealTime::summertime(bool summertime){
  mSummertime = summertime;
}

bool NTPRealTime::init(IPAddress timeserver, int port){
  mUdp.begin(port);
  mTimeServer = timeserver;
}

void NTPRealTime::setSyncInterval(int seconds){
  mSyncInterval = seconds;
}

bool NTPRealTime::fetchNTPTime(){
  sendNTPpacket(mTimeServer);
  delay(300); // 
  // Test for answer 10 times before exit
  for(int i = 0; i < 10; ++i)
    {
      
      if( mUdp.parsePacket() )
	{
	  //buffer to hold incoming and outgoing packets 
	  byte buffer[NTP_PACKET_SIZE]; 
	  // Read packet into the buffer
	  mUdp.read(buffer, NTP_PACKET_SIZE);
	  //the timestamp starts at byte 40 of the received packet and is four bytes,
	  // or two words, long. First, esxtract the two words:
	  unsigned long highWord = word(buffer[40], buffer[41]);
	  unsigned long lowWord = word(buffer[42], buffer[43]);  
	  // combine the four bytes (two words) into a long integer
	  // this is NTP time (seconds since Jan 1 1900):
	  unsigned long secsSince1900 = highWord << 16 | lowWord;               

	  // now convert NTP time into everyday time:
	  // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
	  const unsigned long seventyYears = 2208988800UL;     
	  // subtract seventy years:                            
	  mUnixTime = secsSince1900 - seventyYears;
	  mLastSync = millis();
	  return true;
	}
      delay(300); 
    }
  return false;
}

// send an NTP request to the time server at the given address 
unsigned long NTPRealTime::sendNTPpacket(IPAddress& address)
{
  //buffer to hold incoming and outgoing packets 
  byte buffer[NTP_PACKET_SIZE]; 
  // set all bytes in the buffer to 0
  memset(buffer, 0, NTP_PACKET_SIZE); 
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  buffer[0] = 0b11100011;   // LI, Version, Mode
  buffer[1] = 0;     // Stratum, or type of clock
  buffer[2] = 6;     // Polling Interval
  buffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  buffer[12]  = 49; 
  buffer[13]  = 0x4E;
  buffer[14]  = 49;
  buffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp: 		   
  mUdp.beginPacket(address, 123); //NTP requests are to port 123
  mUdp.write(buffer,NTP_PACKET_SIZE);
  mUdp.endPacket(); 
  return 1;
}

int NTPRealTime::getHour(){
  refreshCache(now());
  // UTC is the time at Greenwich Meridian (GMT)
  // return the hour (86400 equals secs per day)
  return tm.Hour;
}

int NTPRealTime::getMin(){
  refreshCache(now());
  return tm.Minute;
}

int NTPRealTime::getSec(){
  refreshCache(now());
  return tm.Second;
}

time_t NTPRealTime::now(){
  if((millis() - mLastSync)/1000 >= mSyncInterval){
    fetchNTPTime();
  }
  return mUnixTime + (millis() - mLastSync)/1000;
}

void NTPRealTime::breakTime(time_t timeInput){
// break the given time_t into time components
// this is a more compact version of the C library localtime function
// note that year is offset from 1970 !!!

  uint8_t year;
  uint8_t month, monthLength;
  uint32_t time;
  unsigned long days;

  time = (uint32_t)timeInput;
  tm.Second = time % 60;
  time /= 60; // now it is minutes
  tm.Minute = time % 60;
  time /= 60; // now it is hours
  tm.Hour = (time % 24) + mTimezone;
  time /= 24; // now it is days
  tm.Wday = ((time + 4) % 7) + 1;  // Sunday is day 1 
  
  year = 0;  
  days = 0;
  while((unsigned)(days += (LEAP_YEAR(year) ? 366 : 365)) <= time) {
    year++;
  }
  tm.Year = year; // year is offset from 1970 
  
  days -= LEAP_YEAR(year) ? 366 : 365;
  time  -= days; // now it is days in this year, starting at 0
  
  days=0;
  month=0;
  monthLength=0;
  for (month=0; month<12; month++) {
    if (month==1) { // february
      if (LEAP_YEAR(year)) {
        monthLength=29;
      } else {
        monthLength=28;
      }
    } else {
      monthLength = monthDays[month];
    }
    
    if (time >= monthLength) {
      time -= monthLength;
    } else {
        break;
    }
  }
  tm.Month = month + 1;  // jan is month 1  
  tm.Day = time + 1;     // day of month

  if(mSummertime){
    if(tm.Month >= 4 && tm.Month <= 10){
      tm.Hour += 1;
    }
  }
}

