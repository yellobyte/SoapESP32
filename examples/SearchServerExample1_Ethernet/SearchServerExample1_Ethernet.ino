/*
  SearchServerExample1_Ethernet

  The sketch sends a search request for media files that match a single search criteria to all discovered 
  DLNA media servers. The server is not requested to sort the returned list of files.

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
    Serial.println("DHCP ok.");
  }
  else {
    Serial.println("DHCP error !");
    while (true) {
      // no point to continue
    }
  }
  Serial.print("Local IP: ");
  Serial.println(Ethernet.localIP());

  // scan local network for DLNA media servers
  Serial.println();
  Serial.println("Scanning local network for DLNA media servers...");
  soap.seekServer(65);                  // without parameter the scan duration defaults to 60 sec
  Serial.print("Number of discovered servers that deliver content: ");
  Serial.println(soap.getServerCount());
  Serial.println();

  soapObjectVect_t searchResult;        // search results get stored here
  soapServer_t serv;                    // single server info gets stored here
  unsigned int i = 0;                   // start with first entry in server list

  while (soap.getServerInfo(i, &serv)) {
    Serial.print("Server: ");
    Serial.println(serv.friendlyName);
    // send search request to media server: title must contain the string "me"
    if (!soap.searchServer(i, "0", &searchResult, SOAP_SEARCH_CRITERIA_TITLE, "me")) {
      Serial.println("Error searching server.");
    }
    else {
      Serial.print("Search results: ");
      Serial.println(searchResult.size());

      // print each item in result list (if any)
      for (unsigned int j = 0; j < searchResult.size(); j++) {
        Serial.print(" ");
        Serial.print(searchResult[j].name);
        if (searchResult[j].isDirectory) {
          Serial.print("\t (Container, child count: ");
        }
        else {
          Serial.print("\t (Item, size: ");
        }
        if (searchResult[j].sizeMissing) {
          Serial.print("missing");
        }
        else {
          Serial.print(searchResult[j].size);
        }
        if (!searchResult[j].isDirectory) {
          Serial.print(", artist: ");
          Serial.print(searchResult[j].artist);
        }
        Serial.println(")");
      }
    }
    Serial.println("");
    i++;
  }
  Serial.println();
  Serial.println("Sketch finished.");
}

void loop() {
  // 
}
