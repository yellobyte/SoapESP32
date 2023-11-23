/*
  SearchServerExample2_WiFi

  The sketch sends a search request for media files that match two different search criterias to all discovered 
  DLNA media servers. The server is not requested to sort the returned list of files.

  For more info about UPnP search criterias please have a look at file Readme.md.
  Important: Not all media servers support UPnP search requests.

  Last updated 2023-11-22, ThJ <yellobyte@bluewin.ch>
*/

#include <Arduino.h>
#include <WiFi.h>
#include "SoapESP32.h"

const char ssid[] = "MySSID";
const char pass[] = "MyPassword"; 

WiFiClient client;
WiFiUDP    udp;

SoapESP32 soap(&client, &udp);

void setup() {
  Serial.begin(115200);

  // connect to local network via WiFi
  Serial.println();
  Serial.print("Connecting to WiFi network ");
  WiFi.begin(ssid, pass);
  while ( WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("Connected successfully. IP address: ");
  Serial.println(WiFi.localIP());

  // scan local network for DLNA media servers
  Serial.println();
  Serial.println("Scanning local network for DLNA media servers...");
  soap.seekServer();                    // without parameter the scan duration defaults to 60 sec
  Serial.print("Number of discovered servers that deliver content: ");
  Serial.println(soap.getServerCount());
  Serial.println();

  soapObjectVect_t searchResult;        // search results get stored here
  soapServer_t serv;                    // single server info gets stored here
  unsigned int i = 0;                   // start with first entry in server list

  while (soap.getServerInfo(i, &serv)) {
    Serial.print("Server: ");
    Serial.println(serv.friendlyName);    
    // send search request to media server: title must contain the string "hell" & genre must contain the string "rock"
    if (!soap.searchServer(i, "0", &searchResult, SOAP_SEARCH_CRITERIA_GENRE, "rock",
                                                  SOAP_SEARCH_CRITERIA_TITLE, "hell")) {
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
