/*
  SearchServerExample3_WiFi

  The sketch sends a search request for media files that match two different search criterias to all discovered 
  DLNA media servers. The server is requested to sort the returned list of files in descending order.

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
  Serial.print("\nConnecting to WiFi network ");
  WiFi.begin(ssid, pass);
  while ( WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.printf("\nConnected successfully. IP address: %s\n", WiFi.localIP().toString());

  // scan local network for DLNA media servers
  Serial.print("Scanning local network for DLNA media servers...\n");
  soap.seekServer();                    // without parameter the scan duration defaults to 60 sec
  Serial.printf("Number of discovered servers that deliver content: %d\n", soap.getServerCount());

  soapObjectVect_t searchResult;        // search results get stored here
  soapServer_t serv;                    // single server info gets stored here
  unsigned int i = 0;                   // start with first entry in server list

  while (soap.getServerInfo(i, &serv)) {
    Serial.printf("Server: %s\n", serv.friendlyName.c_str());
    // send search request to media server: the object must be an album folder and it's title must contain the string "great" 
    if (!soap.searchServer(i, "0", &searchResult, SOAP_SEARCH_CRITERIA_TITLE, "great",
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
          // since we search for album folders only (SOAP_SEARCH_CLASS_ALBUM) we shouldn't get here
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
