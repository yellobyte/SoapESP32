/*
  BrowseRecursively_Ethernet

  The sketch browses a DLNA media server from root down to a defined 
  sub-directory level. We use a Wiznet W5x00 Ethernet module/shield 
  attached to ESP32 instead of WiFi.
  It's connected to ESP32 GPIO 18, 19, 23 and GPIO 25 (Chip Select).

  Instead of searching via SSDP we set the DLNA media server parameters
  by hand. VLC for example can help to find those parameters. The doc 
  directory holds more infos.
  
  Since memory is limited, by default a maximum of only 100 entries 
  per directory will be returned by browseServer(). This limit is 
  defined in "SoapESP32.h" with parameter SOAP_DEFAULT_BROWSE_MAX_COUNT.
  Increasing this parameter means using more memory.
	
  If a directory contains more than that number, you will have to browse
  that directory multiple times, each time with a higher starting index
  (0, 100, 200,...). 
  Have a look at example "BrowseBigDirectories_WiFi.ino" where this is
  demonstrated.
    
  Last updated 2021-02-02, ThJ <yellobyte@bluewin.ch>
*/

#include <Arduino.h>
#include <Ethernet.h>

#include "SoapESP32.h"

// == IMPORTANT ==
// We use Ethernet module/shield instead of WiFi, so you must do one of the following:
// 1) uncomment line "//#define USE_ETHERNET" in SoapESP32.h OR
// 2) add -DUSE_ETHERNET to compiler.cpreprocessor.flags in platform.txt (ArduinoIDE) OR
// 3) add -DUSE_ETHERNET to your build_flags in platformio.ini (VSCode/PlatformIO)

// uncomment in case you want to know
// #define SHOW_ESP32_MEMORY_STATISTICS

// Please set definitions that apply to your media server / NAS.
// Here are just two examples for server settings:
// 1) Buffalo NAS LinkstationMini, running Twonky media server on Linux.
//    #define SERVER_PORT        9050
//    #define SERVER_CONTROL_URL "TMSContentDirectory/Control"
// 2) Buffalo NAS LS520D, running DiXim media server on Linux.
//    #define SERVER_PORT        55247
//    #define SERVER_CONTROL_URL "dms/control/ContentDirectory"	
	
#define SERVER_IP          192,168,...,...
#define SERVER_PORT        ...
#define SERVER_CONTROL_URL "..."

// How many directory levels to browse (incl. root).
#define BROWSE_LEVELS 3

// MAC address for your Ethernet module/shield 
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

EthernetClient client;
EthernetUDP    udp;

SoapESP32 soap(&client, &udp);

void printServerContent(SoapESP32 *soap, int servNum, String objectId, int numTabs = 0) {
  soapObjectVect_t browseResult;

  if (!soap->browseServer(servNum,          // server number in our internal server list
                          objectId.c_str(), // unique id of object (directory) to search in
                          &browseResult)) { // pointer to vector storing directory content
    Serial.print("error browsing server, object id: ");
    Serial.println(objectId);
    return;
  }
  else {
    for (int i = 0; i < browseResult.size(); i++) {
      // go through each item in list
      for (uint8_t j=0; j<numTabs; j++) {
        Serial.print("  ");                 // indentation
      }
      // print name of item
      Serial.print(browseResult[i].name);
      // recurse for directories, otherwise print size and type of file
      if (browseResult[i].isDirectory ) {
        // directory: append '/' to name
        Serial.println("/");
        if ((numTabs + 1) < BROWSE_LEVELS) { 
          // recurse
          printServerContent(soap, servNum, browseResult[i].id, numTabs + 1);
        }  
      } 
      else {
        // file: append size in bytes and file type
        Serial.print("  ");
        Serial.print("size: ");
        Serial.print(browseResult[i].size, DEC);
        if (browseResult[i].fileType == fileTypeAudio) {
          Serial.println(", audio");
        }
        else if (browseResult[i].fileType == fileTypeVideo) {
          Serial.println(", video");
        }
        else if (browseResult[i].fileType == fileTypeImage) {
          Serial.println(", image");
        }
        else {
          Serial.println(", other");
        }
      }
    }
  }
}

void setup() {
  Serial.begin(115200);

  Ethernet.init(25);    // CS on ESP32 pin 25
  Serial.print("\nInitializing Ethernet...");

  if (Ethernet.begin(mac))
  {
    Serial.println("DHCP ok.");
  }
  else
  {
    Serial.println("DHCP error !");
    while (true) {
      // no point to continue
    }
  }

  Serial.print("Local IP: ");
  Serial.println(Ethernet.localIP());
  Serial.println();

  // add the server to our list
  soap.addServer(IPAddress(SERVER_IP), SERVER_PORT, SERVER_CONTROL_URL);

  // print server content recursively
  Serial.println("----> server content follows:");

  printServerContent(&soap, // pointer to soap object
                     0,     // the first and only server in our server list
                     "0");  // we start with root

#ifdef SHOW_ESP32_MEMORY_STATISTICS
  Serial.println();
  Serial.println("Some ESP32 memory stats after running this sketch:");
  Serial.print(" 1) minimum ever free memory of all regions [in bytes]: ");
  Serial.println(ESP.getMinFreeHeap());
  Serial.print(" 2) minimum ever free heap size [in bytes]:             ");
  Serial.println(xPortGetMinimumEverFreeHeapSize());
  Serial.print(" 3) minimum ever stack size of this task [in bytes]:    ");
  Serial.println(uxTaskGetStackHighWaterMark(NULL)); 
#endif

  Serial.println();
  Serial.println("Sketch finished.");
}

void loop() {
  // 
}

