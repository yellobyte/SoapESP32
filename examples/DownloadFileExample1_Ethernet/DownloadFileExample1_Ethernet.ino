/*
  DownloadFileExample1_Ethernet

  This sketch downloads one file from a DLNA media server and writes it to SD card. 
	
  The parameters needed for download must be set manually further down. 
  You find a snapshot in the doc directory, showing you how to use VLC to find proper values.
  We use a Wiznet W5x00 Ethernet module/shield attached to ESP32 instead of builtin WiFi.

  Ethernet module/shield is attached to GPIO 18, 19, 23 and GPIO 25 (CS).
  SD card module/shield is attached to GPIO 18, 19, 23 and GPIO 5 (CS).
    
  Last updated 2023-10-23, ThJ <yellobyte@bluewin.ch>
*/

#include <Arduino.h>
#include <Ethernet.h>
#include <SD.h>
#include "SoapESP32.h"

// === IMPORTANT ===
// Build option 'USE_ETHERNET' is required for this sketch as we use an Ethernet module/shield. 
// With build option 'SHOW_ESP32_MEMORY_STATISTICS' the sketch prints ESP32 memory stats when finished.
// Both options have already been added to the provided file 'build_opt.h'. Please use it with ArduinoIDE.
// Have a look at Readme.md for more detailed info about setting build options.

// Example settings only, please change:
#define FILE_DOWNLOAD_IP   192,168,1,42
#define FILE_DOWNLOAD_PORT 1557 
#define FILE_DOWNLOAD_URI  "%25/ad9b0c9f4cc1b5940a42962422d62eba/VideoNr2.mp4"

// File download settings
#define FILE_NAME_ON_SD    "/myFile.mp3"
#define READ_BUFFER_SIZE   5000

// MAC address for your Ethernet module/shield
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

#define GPIO_ETHCS 25
#define GPIO_SDCS   5

EthernetClient client;
EthernetUDP    udp;

SoapESP32 soap(&client, &udp);

void setup() {
  Serial.begin(115200);

  Ethernet.init(GPIO_ETHCS);
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

  // preparing SD card 
  Serial.print("Initializing SD card...");
  if (!SD.begin(GPIO_SDCS)) {
    Serial.println("failed!");
    Serial.println("Sketch finished.");
    return;
  }
  Serial.print("done. Creating file on SD ");  
  File myFile = SD.open(FILE_NAME_ON_SD, FILE_WRITE);
  if (!myFile) {
    Serial.println("failed!");
    return;
  }
  Serial.println("was successful."); 

  // memory allocation for read buffer
  uint8_t *buffer = (uint8_t *)malloc(READ_BUFFER_SIZE);
  if (!buffer) {
    Serial.println("malloc() error!");    
    return;
  }

  size_t fileSize;          // file size announced by server
  uint32_t bytesRead;       // read count
  soapObject_t object;      // holds all necessary infos for download

  object.isDirectory  = false;
  object.downloadIp   = IPAddress(FILE_DOWNLOAD_IP);
  object.downloadPort = FILE_DOWNLOAD_PORT;
  object.uri          = FILE_DOWNLOAD_URI;

  // attempting to download a file bigger than 4GB will fail !
  if (!soap.readStart(&object, &fileSize)) {
    // Error
    Serial.println("Error requesting file from media server.");
  }
  else {
    // request for file download was granted from server
    Serial.println();
    Serial.print("Download request granted from server, announced file size: ");
    Serial.println(fileSize);
    Serial.println("Start copying file from server to SD, please wait."); 
    delay(1000);
    
    bytesRead = 0;
    do {
      int res = soap.read(buffer, READ_BUFFER_SIZE);
      if (res < 0) {
        // read error  
        break;
      }         
      else if (res > 0) {
        // Remark: At this point instead of writing to SD card you could write the data 
        // into a buffer/queue which feeds an audio codec (e.g. VS1053) for example
        if (!myFile.write(buffer, res)) {
          Serial.println("Error writing to SD card."); 
          break;
        }
        //
        bytesRead += res;
        Serial.print(".");
      }  
      else { 
        // res == 0, momentarily no data available
      }
    } 
    while (soap.available());
    Serial.println();

    // close connection to server
    soap.readStop();

    Serial.println();
    if (bytesRead == fileSize) {
      Serial.println("File download was successful.");     
    }
    else {
      Serial.println("Error downloading file.");
    }
  }

  free(buffer);
  myFile.close();
  Serial.println("File on SD closed.");

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
