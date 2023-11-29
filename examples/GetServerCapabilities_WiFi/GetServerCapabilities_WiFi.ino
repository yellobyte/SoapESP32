/*
  GetServerCapabilities_WiFi

  This sketch scans the local network for DLNA media servers via builtin WiFi
  and queries the search/sort capabilities of each server found.

  Last updated 2023-11-28, ThJ <yellobyte@bluewin.ch>
*/

#include <Arduino.h>
#include <WiFi.h>
#include "SoapESP32.h"

// With build option 'SHOW_ESP32_MEMORY_STATISTICS' the sketch prints ESP32 memory stats when finished.
// The option has already been added to the provided file 'build_opt.h'. Please use it with ArduinoIDE.
// Have a look at Readme.md for more detailed info about setting build options.

#define NETWORK_SCAN_DURATION 70

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
  soap.seekServer(NETWORK_SCAN_DURATION);  
  Serial.print("Number of discovered servers that deliver content: ");
  Serial.println(soap.getServerCount());
  Serial.println();

  soapServerCapVect_t result;        // reported capabilities get stored here
  soapServer_t        serv;          // single server info gets stored here
  unsigned int        i = 0;         // start with first entry in server list

  while (soap.getServerInfo(i, &serv)) {
    // Print some server details
    Serial.print("Server[");
    Serial.print(i);
    Serial.print("]: IP address: ");
    Serial.print(serv.ip);
    Serial.print(", port: ");
    Serial.print(serv.port);
    Serial.print(", name: ");
    Serial.println(serv.friendlyName);

    // query the server's search capabilities 
    if (!soap.getServerCapabilities(i, capSearch, &result)) {
      Serial.println("Error querying search capabilities from server.");
    }
    else {
      Serial.print("Server has reported search capabilities: ");
      Serial.println(result.size());
      for (unsigned int j = 0; j < result.size(); j++) {
        Serial.print(" ");
        Serial.println(result[j]);
      }
    }
    Serial.println("");

    if (result.size()) {
      // querying a server's sort capabilities is only useful if he provides any search capabilities
      if (!soap.getServerCapabilities(i, capSort, &result)) {
        Serial.println("Error querying sort capabilities from server.");
      }
      else {
        Serial.print("Server has reported sort capabilities: ");
        Serial.println(result.size());
        for (unsigned int j = 0; j < result.size(); j++) {
          Serial.print(" ");
          Serial.println(result[j]);
        }
      }
    }

    Serial.println("");    
    i++;
  }

#ifdef SHOW_ESP32_MEMORY_STATISTICS
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
