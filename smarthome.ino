

/* Serial: ttyACM1
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
#include <AVL_tree.h>

#define transmitPin 10

/*
 * CACHE_SIZE can't be more than 550. If it is the cache 
 * won't fit in the EEPROM.
 */
#define CACHE_SIZE 40
#define TIMER_CHECK_INTERVAL 30 // Seconds
#define DHCP_RENEW_INTERVAL 60 // Seconds
#define EMPTY 255

byte mac[] = {  
  0x00, 0x26, 0x77, 0xA4, 0xF7, 0x4C };
// IPAddress ip(192, 168, 1, 151);
const unsigned int localPort = 8888;
EthernetServer server(localPort);

AVL_tree* tree;
RCTransmit transmit = RCTransmit(transmitPin);
NTPRealTime ntp = NTPRealTime();
IPAddress timeServer(132, 163, 4, 101);

unsigned long lastTimerCheck; // Seconds
unsigned long lastDHCPRenew; // Seconds

void readRequest(EthernetClient* client, char* request);
void executeRequest(EthernetClient* client, char* request);
void sendResponse(EthernetClient* client, String response);
boolean setTimer(char* request);
void checkTimers(TreeNode*& node);
boolean timeToCheckTimers();
boolean maintainDHCP();

void setup()
{
  // EEPROM.write(0, 0);
  Serial.println(F("Setup initialized!"));
   // Setup serial for debug
  Serial.begin(9600);
  // Setup ethernet and server
  // Ethernet.begin(mac,ip) ;
  while( Ethernet.begin(mac) != 1 ){
    delay(5000);
  }
  server.begin();
  Serial.print(F("Server address:"));
  Serial.println(Ethernet.localIP());

  // Setup RCtransmit
  transmit.setRepeatTransmit(5);
  // Setup NTP RealTime
  ntp.init(timeServer, localPort);
  ntp.setSyncInterval(300);
  ntp.summertime(true);
  // Load avl-cache...
  tree = new AVL_tree(CACHE_SIZE);
  Serial.println(F("Cache initialized!"));
  Serial.println(F("Setup finished!"));
}

void loop()
{
  EthernetClient client = server.available();
  
  // Check if any timers are go
  if(timeToCheckTimers())
    tree->ForEach(checkTimers);

  // Renew DHCP lease
  maintainDHCP();

  // Check if any requests
  if(!client)
  {
    delay(100);
    return ;
  }
  
  Serial.println(F("Client connected!"));
  // Handle client
  char request[80];
  readRequest(&client, request);
  executeRequest(&client, request);

  // Close connection
  client.stop();
  Serial.println(F("Client disconnected!"));
}

// Read request line
void readRequest(EthernetClient* client, char* request)
{
  //String request = "";
  //char request[80];
  int i = 0;
  //Loop while the client is connected
  while(client->connected())
  {
    // Read available bytes
     while(client->available())
    {
       // Read byte
       char c = client->read();
       
       //Serial.write(c);
       
       // Exit if end of line
       if('\n' == c || '\0' == c)
       {
         request[i] = '\0';
         //char* d = request;
         //return &request[0]; 
	 return;
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
  Serial.println(F("######### Serving client ############"));
  Serial.print(F("Command: "));
  Serial.println(command[0]);
  switch(command[0])
    {
    case 'S': //Switch on/off
      {
	// Get controller code
	byte controller = 0;
	byte on = 0;
	controller = byte(atoi(strtok_r(request, ":", &request)));
	on = atoi(strtok_r(request, ":", &request));
	if(on == 1){
	  transmit.switchOn(controller, 2, 0, 0);
	  tree->SetStatus(controller, 1);
	}
	else{
	  transmit.switchOff(controller, 2, 0, 0);
	  tree->SetStatus(controller, 0);
	}
	sendResponse(client, "OK");
	break;
      }
    case 'G': // Send all saved nodes
      {
        tree->SendNodes(client);
	break;
      } 
    case 'A': // Add switch
      {
	byte id = byte(atoi(strtok_r(request, ":", &request)));
	if( id > 0 && id < 255 && tree->Insert(id))
	  {
	    sendResponse(client, "OK");
	  }
	else
	  {
	    sendResponse(client, "NOK");
	  }
	break;
      } 
    case 'R': // Remove switch states
      {
	byte id = byte(atoi(strtok_r(request, ":", &request)));
	if( id > 0 && id < 255 && tree->Remove(id) )
	  {
	    sendResponse(client, "OK");
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
      case 'T': // Set/Add Timer => timerid:onHour:onMinute:offHour:offMinute:switchidN:switchidK:....:switchidZ:
      {
        if( setTimer(request) ){
	  sendResponse(client, "OK");
	}else{
	  sendResponse(client, "NOK");
	}
	break;
      }
      case 'Q': // Remove Timer => timerid
      {
	byte timerid = atoi(strtok_r(request, ":", &request));
        tree->RemoveTimer(timerid);
	sendResponse(client, "OK");
	break;
      }
    default:
      {
	sendResponse(client, "NO SUCH COMMAND EXIST");
	break;
      }
    }
}

void sendResponse(EthernetClient* client, String response)
{
	// Send response to client.
	client->println(response);
	// Debug print.
	Serial.print(F("sendResponse: "));
	Serial.println(response);
}

// Set Timer => timerid:onHour:onMinute:offHour:offMinute:switchidN:switchidK:....:switchidZ:
boolean setTimer(char* request){
  //byte(atoi());
  byte timerid = atoi(strtok_r(request, ":", &request));
  byte onHour = atoi(strtok_r(request, ":", &request));
  byte onMinute = atoi(strtok_r(request, ":", &request));
  byte offHour = atoi(strtok_r(request, ":", &request));
  byte offMinute = atoi(strtok_r(request, ":", &request));
  Serial.print(F("Adding timer with id: "));
  Serial.print(timerid);
  Serial.print(F(" onHour: "));
  Serial.print(onHour);
  Serial.print(F(" onMinute: "));
  Serial.print(onMinute);
  Serial.print(F(" offHour: "));
  Serial.print(offHour);
  Serial.print(F(" offMinute: "));
  Serial.println(offMinute);
  byte switchids[tree->Size()+1];
  byte i = 0;
  while(request){
    byte swId = byte(atoi(strtok_r(request, ":", &request)));
    if(swId >= 10 && swId <= 250){
      switchids[i++] = swId;
      Serial.print("Switch: ");
      Serial.println(switchids[i-1]);
    }
  }
  switchids[i] = 0; // Mark end
  byte * p = switchids;
  tree->SetTimer(p, timerid, onHour, onMinute, offHour, offMinute);
  return true;
}

boolean timeToCheckTimers()
{
  if(!((millis()/1000)-lastTimerCheck >= TIMER_CHECK_INTERVAL))
    return false;

  Serial.print(F("Time: "));
  Serial.print(ntp.getHour());
  Serial.print(F(":"));
  Serial.println(ntp.getMin());

  lastTimerCheck = millis()/1000;
  return true;
}

void checkTimers(TreeNode*& node){
  Serial.print(F("Checking timer for id: "));
  Serial.println(node->d);
  // If not recent
  if(node->timerid != EMPTY)
    {
      Serial.print(F(" Had timer! Time on: "));
      Serial.print(node->onHour);
      Serial.print(F(":"));
      Serial.print(node->onMinute);
      Serial.print(F(" Time off: "));
      Serial.print(node->offHour);
      Serial.print(F(":"));
      Serial.print(node->offMinute);
      Serial.println(F("."));
      if( int(node->offHour) == int(ntp.getHour()) &&  int(node->offMinute) == int(ntp.getMin()))
	{
	  transmit.switchOff(node->d,1, 0, 0);
	  node->status = false;
	}
      else if( int(node->onHour) == int(ntp.getHour()) && int(node->onMinute) == int(ntp.getMin()))
	{
	  transmit.switchOn(node->d,1, 0, 0);
	  node->status = true;
	}
    }
}

boolean maintainDHCP(){
  if((millis()/1000)-lastDHCPRenew >= DHCP_RENEW_INTERVAL){
    lastDHCPRenew = millis()/1000;
    int returnCode = Ethernet.maintain();
    while (!(returnCode == 0 || returnCode == 2)){
      switch(returnCode) {
      case 1:   Serial.print(F("\n\rDHCP: Renew failed")); break;
      case 3:   Serial.print(F("\n\rDHCP: Rebind fail")); break;
      case 4:   Serial.print(F("\n\rDHCP: Rebind success")); break;
      }
      delay(5000);
      returnCode = Ethernet.maintain();
    }
  }
  return true;
}