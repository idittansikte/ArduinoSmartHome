

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
#define CACHE_SIZE 70
#define TIMER_CHECK_INTERVAL 30 // Seconds
#define EMPTY 255

byte mac[] = {  
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 151);
const unsigned int localPort = 8888;
EthernetServer server(localPort);

AVL_tree* tree;
RCTransmit transmit = RCTransmit(transmitPin);
NTPRealTime ntp = NTPRealTime();
IPAddress timeServer(132, 163, 4, 101);

unsigned long lastTimerCheck; // Seconds

void readRequest(EthernetClient* client, char* request);
void executeRequest(EthernetClient* client, char* request);
void sendResponse(EthernetClient* client, String response);
boolean setTimer(char* request);
void checkTimers(TreeNode* node);

void setup()
{
  // EEPROM.write(0, 0);
  Serial.println(F("Setup initialized!"));
   // Setup serial for debug
  Serial.begin(9600);
  // Setup ethernet and server
  Ethernet.begin(mac,ip) ;
  server.begin();
  Serial.print(F("Server address:"));
  Serial.println(Ethernet.localIP());
  // Setup RCtransmit
  transmit.setProtocol(1);
  transmit.setPulseLength(260);
  transmit.setRepeatTransmit(6);
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
  tree->ForEach(checkTimers);
  //char tmp[] = "66:23:00:11:30:1:2:3:4:5:6";
  //setTimer(tmp);
  //delay(10000);
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
  Serial.println(F("#####################"));
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
	  transmit.switchOn(controller, 0, 0);
	  tree->SetStatus(controller, 1);
	}
	else{
	  transmit.switchOff(controller, 0, 0);
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
  Serial.println(timerid);
  Serial.print(F(" onHour: "));
  Serial.println(onHour);
  Serial.print(F(" onMinute: "));
  Serial.println(onMinute);
  Serial.print(F(" offHour: "));
  Serial.println(offHour);
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

void checkTimers(TreeNode* node){
  // If recent
  //Serial.print("Time since last check: ");
  //Serial.println((millis()/1000)-lastTimerCheck);
  if(!((millis()/1000)-lastTimerCheck >= TIMER_CHECK_INTERVAL))
    return;
  Serial.print(F("Time: "));
  Serial.print(ntp.getHour());
  Serial.print(F(":"));
  Serial.println(ntp.getMin());
  // If not recent
  if(node->timerid != EMPTY)
    {
      if( node->offHour == ntp.getHour() )
	{
	  if( node->offMinute == ntp.getMin() )
	    {
	      transmit.switchOff(node->d, 0, 0);
	      node->status = false;
	    }
	}
      else if( node->onHour == ntp.getHour() )
	{
	  if( node->onMinute == ntp.getMin() )
	    {
	      transmit.switchOn(node->d, 0, 0);
	      node->status = true;
	    }
	}
    }
  lastTimerCheck = millis()/1000;
}