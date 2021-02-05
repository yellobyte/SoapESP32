# SoapESP32

This Arduino library provides basic UPnP/SOAP functionality, enabling an ESP32 device to scan the local network for DLNA media servers, scan their contents and finally retrieve files.

Motivation for writing it was the missing capability of my ESP32-Radio (based on Ed Smallenburg's great project) to play audio content stored on the various NAS devices in the local network at home (most of them running a DLNA server).

The library has been tested so far with the following DLNA media servers: 

  - **DiXim** and **Twonky** running on Linux (NAS devices) and 
  - **UMS** (Universal Media Server) & **Windows Media Player** running on Win10 (in a virtual machine)
	
If you run into trouble with your particular DLNA media server or NAS, increase `CORE_DEBUG_LEVEL` and it gives you an indication where the problem is. Tracing the communication with Wireshark helps a lot!

## Using W5x00 Ethernet shield/boards instead of builtin WiFi

Using a Wiznet W5x00 board and the standard Arduino Ethernet lib for communication produced some sporadic issues. Especially client.read() calls returned corrupted data every now and then, esp. with other threads using the SPI bus simultaneously.
This problem is mentioned a few times in forums on the internet, so it seems to be a known ESP32 Arduino SPI library issue.

The only remedy I found was to wrap all function calls that use SPI with a global/project wide mutex lock (realized within this library with the aid of claimSPI()/releaseSPI()). This completely wiped all those problems and none of the garbling issues happened ever again. 
See example UsingMutexLocks_Ethernet for more infos.

Of course, the ESP32 Arduino SPI library already uses locks (SPI_MUTEX_LOCK/_UNLOCK) but doesn't seem to be 100% thread proof though. Please correct me if I'm wrong or you do find a better solution.

## SSDP multicast

SSDP M-SEARCH multicast packets carry a destination ip 239.255.255.250 and destination port 1900.
However, some NAS devices (e.g. Buffalo's NAS LS520D) unexpectedly do NOT reply to such packets when the **source** port is 1900 as well.  
Some articles on the internet state that NAS devices might show this behaviour to prevent SSDP (r)DDoS storms.

Unfortunately when using the standard Arduino Ethernet library (instead of WiFi) all SSDP multicast packets carry identical destination & source ports, so in our case 1900. Therefore my Buffalo NAS LS520D never responded to SSDP search requests when using an Ethernet module/shield.

### There are two solutions to this problem:

a) use addServer() to manually add a server to the server list (which I recommend!) **or**

b) modify "socket.cpp" in Arduino Ethernet library to use a source port between 49152 & 65535. Which is not only a dirty solution but puts you under risk to forget about it and then loose those changes when updating the Ethernet lib a year later in a hurry.  

```c
uint8_t EthernetClass::socketBeginMulticast(uint8_t protocol, IPAddress ip, uint16_t port)
{
  ...
  ...
  if (port > 0 && (protocol != (SnMR::UDP | SnMR::MULTI))) {	 // <------ modification 
    W5100.writeSnPORT(s, port);
  } else {
    // if don't set the source port, set local_port number.
    if (++local_port < 49152) local_port = 49152;
    W5100.writeSnPORT(s, local_port);
  }
  // Calculate MAC address from Multicast IP Address
  byte mac[] = {  0x01, 0x00, 0x5E, 0x00, 0x00, 0x00 };
  ...
```

## How to find correct server parameters needed in some examples

Four of the examples, namely 
* _BrowseRecursively_Ethernet.ino_
* _BrowseRecursively_WiFi.ino_
* _DownloadFileExample1_Ethernet.ino_
* _DownloadFileExample1_WiFi.ino_

require some parameters (that apply to your specific DLNA media server) be defined manually.  

The 3 snapshots _Using_VLC_to_find_....._parameter.JPG_ in folder **Doc** show you how to use the program **VLC** to find proper values.

## Compiling the examples

All examples were compiled & tested with ArduinoIDE V1.8.9 and VSCode V1.52.1/PlatformIO Core5.0.4/Home 3.3.1.

If possible, please always set project wide preprocessor option `__GNU_VISIBLE` which enables usage of strcasestr() provided in _string.h_. 
However, if `__GNU_VISIBLE` is not defined, a quick and dirty version of strcasestr() defined in _SoapESP32.cpp_ will be used instead.

If you use an Ethernet module/shield instead of builtin WiFi you MUST set the preprocessor option `USE_ETHERNET`.  

Unfortunately we can't set project wide preprocessor/compiler options in *.ino sketches though. 

### So in case you use the Arduino IDE:

Add any needed additional options to line **compiler.cpreprocessor.flags** in your Arduino IDE file _platform.txt_.  
On my PC for example I find this file in the following directory:  	
* _C:\Users\tj\AppData\Local\Arduino15\packages\esp32\hardware\esp32\1.0.4_

But be reminded, those options will stay permanent until you delete them.  
So having `-DUSE_ETHERNET` in _platform.txt_ does NOT work with examples that use WiFi (..._WiFi.ino).  

Alternatively you could just uncomment the line **//#define USE_ETHERNET** in _SoapESP32.h_. Whatever you prefer.

### And in case you use VSCode/PlatformIO:

You are lucky. Simply add the options to your _platformio.ini_ project file:

build_flags = `-D__GNU_VISIBLE, -DUSE_ETHERNET`
	
## Documentation

Folder **Doc** contains various files to help you implement this library into your projects:
* log files (Serial Monitor output) of all examples, some with different core debug levels
* Schematic diagram (wiring) plus picture of the test set used
* VLC snapshots to help you find server & file download parameters needed for some examples
* the platformio.ini file I used when testing examples with VSCode/PlatformIO

## Example for an implementation: ESP32-Radio

After merging this library with an existing ESP32-Radio it's been running for a few month now without any problems. Biggest advantage is not having to fiddle around with SD cards anymore!  

![github](https://github.com/yellobyte/SoapESP32/raw/main/doc/ESP32-Radio-DLNA.jpg)

Alternatively have a look at the short clip _ESP32-Radio-DLNA.mp4_ in folder **Doc** to see the final implementation in action.