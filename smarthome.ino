

/*
* Transmitting
* 
* Starts with high for DELAY_SHORT (260) us and low for DELAY_START (2675) us
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
*
*
*
* EEPROM
* Address 0:
* First address is for storing how many switch_cache there is
* Address 1-400:
* Max 200 switch_cache, each switch require two bytes 
*/

#include <SPI.h>
#include <Ethernet.h>
#include <EEPROM.h>
#include <RCTransmit.h>
#include <NTPRealTime.h>

#define transmitPin 10

/*
 * CACHE_SIZE can't be more than 550. If it is the cache 
 * won't fit in the EEPROM.
 */
#define CACHE_SIZE 124

byte mac[] = {  
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 151);
const unsigned int localPort = 8888;
EthernetServer server(localPort);

RCTransmit transmit = RCTransmit(transmitPin);
NTPRealTime ntp = NTPRealTime();
IPAddress timeServer(132, 163, 4, 101);

typedef struct 
{
  int controller;
  boolean status;
  int hour;
  int minute;
} switch_type;

switch_type switch_cache[CACHE_SIZE];
boolean cacheStringUptoDate=false;
String cacheString;

void setup()
{
   // Setup serial for debug
  Serial.begin(9600);
  // Setup ethernet and server
  Ethernet.begin(mac,ip) ;
  server.begin();
  Serial.print("Server address:");
  Serial.println(Ethernet.localIP());
  // Setup RCtransmit
  transmit.setProtocol(1);
  transmit.setPulseLength(260);
  transmit.setRepeatTransmit(6);
  // Setup NTP RealTime
  ntp.init(timeServer, localPort);
  ntp.setSyncInterval(300);
  ntp.summertime(true);
  // Init cahc and load switchen from EEPROM into it
  initCache();
  loadSwitchesFromMemory();
  Serial.println("Cache initialized!");
  Serial.println("Setup finished!");
}

//char test[] = "S:35235:1";

void loop()
{
  EthernetClient client = server.available();
  if(!client)
  {
    delay(100);
    return ;
  }
  
  Serial.println("Client connected!");
  // Handle client
  char* request = readRequest(&client);
  executeRequest(&client, request);
  // Close connection
  client.stop();
  Serial.println("Client disconnected!");
}

// Read request line
char* readRequest(EthernetClient* client)
{
  //String request = "";
  char request[80];
  int i = 0;
  //Loop while the client is connected
  while(client->connected())
  {
    // Read available bytes
     while(client->available())
    {
       // Read byte
       char c = client->read();
       
       Serial.write(c);
       
       // Exit if end of line
       if('\n' == c || '\0' == c)
       {
         request[i] == '\0';
         char* d = request;
         return d; 
       }
       
       // Add byte to request line
       request[i] = c;
       ++i;
    } 
  }
}

void executeRequest(EthernetClient* client, char* request)
{
  char* command  = strtok_r(request, ":", &request);
  Serial.print("Command: ");
  Serial.println(command[0]);
  switch(command[0])
    {
    case 'S': //Switch on/off
      {
	// Get controller code
	unsigned controller = 0;
	unsigned int on = 0;
	controller = atoi(strtok_r(request, ":", &request));
	on = atoi(strtok_r(request, ":", &request));
	if(on == 1)
	  transmit.switchOn(controller, 0, 0);
	else
	  transmit.switchOff(controller, 0, 0);

	sendResponse(client, "OK");
	// Save switch state
        addSwitchToCache(controller);
	cacheSwitchStatus(controller, on);
	break;
      }
    case 'G': // Send back saved switch_cache and their states
      {
        if(!cacheStringUptoDate)
          cacheToString();
          
	sendResponse(client, cacheString);
	break;
      } 
    case 'A': // Add switch and its states
      {
	if(addSwitchToCache(atoi(strtok_r(request, ":", &request))))
	  {
	    sendResponse(client, "OK");
	    saveSwitchesToMemory();
	  }
	else
	  {
	    sendResponse(client, "NOK");
	  }
	break;
      } 
    case 'R': // Remove switch states
      {
	if(removeSwitchFromCache(atoi(strtok_r(request, ":", &request))))
	  {
	    sendResponse(client, "OK");
	    saveSwitchesToMemory();
	  }
	else
	  {
	    sendResponse(client, "NOK");
	  }
	break;
      } 
      case 'C': // Check connectivity
      {
         sendResponse(client, "OK");
         break;
      }
      case 'T': // Set Timers
    default:
      {
	sendResponse(client, "NO SUCH COMMAND EXIST");
	break;
      }
    }
}

void cacheSwitchStatus(unsigned controller, int status)
{
  int i = 0;
  for(; i < CACHE_SIZE; ++i) // Go through cache
    {
      //If controller found
      if(switch_cache[i].controller == controller)
	{
             // Set status
            if (status == 1)
	      switch_cache[i].status = true;
            else
              switch_cache[i].status = false;
              
            cacheStringUptoDate = false;
	}
    }
}

/*
 * Add switch to cache.
 * Returns false is there is no room left, else true.
 */
boolean addSwitchToCache(int controller)
{
  int pos = 0;
  int savedPos = -1;
  // Find empty pos and look for identical switch
  for(; pos < CACHE_SIZE; ++pos)
    {
      if(switch_cache[pos].controller == -1) // If empty
	{
	  savedPos = pos; // store empty pos to later
	}
      else if (switch_cache[pos].controller == controller) // If controller already exists
      {
         return false; 
      }
    }
    
    // If not full
  if(savedPos != -1)
    {
      // Save switch in cache
      switch_cache[savedPos].controller = controller;
      switch_cache[savedPos].status = false;
      Serial.println("Controller added.");
      cacheStringUptoDate=false;
      return true;
    }
    
  // If this is the chache is full and we dont add it...
  return false;
}

/*
 * Remove switch from cache.
 * Returns false if not found else true.
 */
boolean removeSwitchFromCache(int controller)
{
  int position = 0;
  for(; position < CACHE_SIZE; ++position)
    {
      if(switch_cache[position].controller == controller) // If found 
	{
	  switch_cache[position].controller = -1; // Remove it
          cacheStringUptoDate=false;
	  return true;
	}
    }
    // If not f
  return false;
}

void cacheToString()
{
  cacheString = "";
  for(int i = 0; i < CACHE_SIZE; ++i) // For each entry in chache
    {
      if(switch_cache[i].controller != -1) // If controller at position
	{
          // Append it to cacheString...
  	  cacheString += switch_cache[i].controller;
          cacheString += ":";
          if(switch_cache[i].status)
            cacheString += "1";
          else
            cacheString += "0";
          cacheString += ":";
        }
    }
  cacheStringUptoDate=true;
}

/*
 * Save switch_cache in cache into EEPROM
 * This is done when any changes have been done to cache.
 */
void saveSwitchesToMemory()
{
  int addr = 1; // Current EEPROM address
  int position = 0; // Array position
  while(position < CACHE_SIZE)
  {
    int controller = switch_cache[position].controller;
    boolean status = switch_cache[position].status;
    if(controller != -1){
      EEPROM.write(addr++, controller);
    }
    ++position;
    EEPROM.write(0, addr-1);
  }
}

/*
 * Load switch_cache in EEPROM into cache.
 * This should only be done at setup state.
 */
void loadSwitchesFromMemory()
{
  int count = EEPROM.read(0); // How many switch_cache in memory
  int addr = 1; // Current EEPROM address
  int pos = 0; // switch_cache pos
  while(addr <= count)
  {
     // Read controller
     switch_cache[pos++].controller = EEPROM.read(addr++);
  }
}

void sendResponse(EthernetClient* client, String response)
{
	// Send response to client.
	client->println(response);
	// Debug print.
	Serial.print("sendResponse:");
	Serial.println(response);
}

void initCache()
{
   for(int i = 0; i < CACHE_SIZE; ++i)
   {
      switch_cache[i].controller = -1;
      switch_cache[i].hour = -1;
      switch_cache[i].minute = -1;
   } 
}
