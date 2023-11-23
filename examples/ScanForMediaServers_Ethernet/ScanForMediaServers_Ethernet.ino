/*
  ScanForMediaServers_Ethernet

  This sketch scans the local network for DLNA media servers and prints them.
	
  We use a Wiznet W5x00 Ethernet module/shield instead of builtin WiFi.
  It's connected to ESP32 GPIO 18, 19, 23 and GPIO 25 (Chip Select).

  Last updated 2023-11-22, ThJ <yellobyte@bluewin.ch>
 */

#include <Arduino.h>
#include <Ethernet.h>
#include "SoapESP32.h"

// === IMPORTANT ===
// Build option 'USE_ETHERNET' is required for this sketch as we use an Ethernet module/shield. 
// With build option 'SHOW_ESP32_MEMORY_STATISTICS' the sketch prints ESP32 memory stats when finished.
// Both options have already been added to the provided file 'build_opt.h'. Please use it with ArduinoIDE.
// Have a look at Readme.md for more detailed info about setting build options.

// Ethernet module/shield settings
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
#define GPIO_ETHCS 25

EthernetClient client;
EthernetUDP    udp;

SoapESP32 soap(&client, &udp);

void setup() {
  Serial.begin(115200);
  
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
  soap.seekServer(50);          // scan duration set to 50 sec
  Serial.print("Number of discovered servers that deliver content: ");
  Serial.println(soap.getServerCount());
  Serial.println();

  // Show connection details of all discovered, usable media servers
  soapServer_t srv;
  unsigned int srvNum = 0;

  while (soap.getServerInfo(srvNum++, &srv)) {
    // print server details
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
