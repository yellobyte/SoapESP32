/*
  ScanForMediaServers_WiFi

  This sketch scans the local network for DLNA media servers
  using ESP32 builtin WiFi and prints them.

  Last updated 2023-10-23, ThJ <yellobyte@bluewin.ch>
 */

#include <Arduino.h>
#include <WiFi.h>
#include "SoapESP32.h"

// With build option 'SHOW_ESP32_MEMORY_STATISTICS' the sketch prints ESP32 memory stats when finished.
// The option has already been added to the provided file 'build_opt.h'. Please use it with ArduinoIDE.
// Have a look at Readme.md for more detailed info about setting build options.

const char ssid[] = "MySSID";
const char pass[] = "MyPassword"; 

WiFiClient client;
WiFiUDP    udp;

SoapESP32 soap(&client, &udp);

void setup() {
  Serial.begin(115200);
  delay(10);

  // Connect to local network via WiFi
  Serial.println();
  Serial.print("Connecting to WiFi network ");
  WiFi.begin(ssid, pass);
  while ( WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.println("Connected successfully.");
  Serial.print("IP address: "); 
  Serial.println(WiFi.localIP());

  // scan local network for DLNA media servers
  Serial.println();
  Serial.println("Scanning local network for DLNA media servers...");
  soap.seekServer();            // without parameter the scan duration defaults to 60 sec
  Serial.print("Number of discovered servers that deliver content: ");
  Serial.println(soap.getServerCount());
  Serial.println();

  // Show connection details of all discovered, usable media servers
  soapServer_t srv;
  uint8_t srvNum = 0;

  while (soap.getServerInfo(srvNum++, &srv)) {
    // Print server details
    Serial.print("Server[");
    Serial.print(srvNum);
    Serial.print("]: IP address: ");
    Serial.print(srv.ip);
    Serial.print(", port: ");
    Serial.print(srv.port);
    Serial.print(", name: ");
    Serial.println(srv.friendlyName);
    Serial.print("  -> controlURL: ");
    Serial.println(srv.controlURL);
  }

#ifdef SHOW_ESP32_MEMORY_STATISTICS
  Serial.println("");
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
