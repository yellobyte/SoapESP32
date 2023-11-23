/*
  SearchServerExample3_Ethernet

  The sketch sends a search request for media files that match two different search criterias to all discovered 
  DLNA media servers. The server is requested to sort the returned list of files in descending order.

  For more info about UPnP search criterias please have a look at file Readme.md.
  Important: Not all media servers support UPnP search requests.

  We use a Wiznet W5x00 Ethernet module/shield instead of builtin WiFi. 
  It's attached to GPIO 18, 19, 23 and GPIO 25 (Chip Select).

  Last updated 2023-11-22, ThJ <yellobyte@bluewin.ch>
*/

#include <Arduino.h>
#include <Ethernet.h>
#include "SoapESP32.h"

// === IMPORTANT ===
// Build option 'USE_ETHERNET' is required for this sketch as we use an Ethernet module/shield. 
// The options has already been added to the provided file 'build_opt.h'. Please use it with ArduinoIDE.
// Have a look at Readme.md for more detailed info about setting build options.

// Ethernet module/shield settings
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
#define GPIO_ETHCS 25

EthernetClient client;
EthernetUDP    udp;

SoapESP32 soap(&client, &udp);

void setup() {
  Serial.begin(115200);

  // getting Ethernet up and running
  Ethernet.init(GPIO_ETHCS);
  Serial.print("\nInitializing Ethernet...");

  if (Ethernet.begin(mac)) {
    Serial.print("DHCP ok.\n");
  }
  else {
    Serial.print("DHCP error !\n");
    while (true) {
      // no point to continue
    }
  }
  Serial.printf("Local IP: %s\n", Ethernet.localIP().toString());

  // scan local network for DLNA media servers
  Serial.print("Scanning local network for DLNA media servers...\n");
  soap.seekServer();                    // without parameter the scan duration defaults to 60 sec
  Serial.printf("Number of discovered servers that deliver content: %d\n", soap.getServerCount());

  soapObjectVect_t searchResult;        // search results get stored here
  soapServer_t serv;                    // single server info gets stored here
  unsigned int i = 0;                   // start with first entry in server list

  while (soap.getServerInfo(i, &serv)) {
    Serial.printf("Server: %s\n", serv.friendlyName.c_str());
    // send search request to media server: the object must be an album folder and it's title must contain the string "best" 
    if (!soap.searchServer(i, "0", &searchResult, SOAP_SEARCH_CRITERIA_TITLE, "best",
                                                  SOAP_SEARCH_CRITERIA_CLASS, SOAP_SEARCH_CLASS_ALBUM,
                                                  SOAP_SORT_TITLE_DESCENDING)) {
      Serial.print("Error searching server.\n");
    }
    else {
      Serial.printf("Search results: %d\n", searchResult.size());
      // print each item in result list (if any)
      for (unsigned int j = 0; j < searchResult.size(); j++) {
        Serial.printf(" %s", searchResult[j].name.c_str());
        if (searchResult[j].isDirectory) {
          Serial.printf("\t (Container, id=%s, child count: %llu)\n", searchResult[j].id.c_str(), searchResult[j].size);
        }
        else {
          // since we search for album folder only (SOAP_SEARCH_CLASS_ALBUM) we shouldn't get here
          Serial.printf("\t (Item, size: %llu, artist: %s)\n", searchResult[j].size, searchResult[j].artist.c_str());
        }
      }
    }
    Serial.print("\n");
    i++;
  }
  Serial.print("\nSketch finished.\n");
}

void loop() {
  // 
}
