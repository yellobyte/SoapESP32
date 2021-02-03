# SoapESP32

This Arduino library provides only the most basic & simple UPnP/SOAP functionality, enabling an ESP32 device to scan the local network for DLNA media servers, scan their contents and finally retrieve files.

Since EPS32 speed and memory are limited, focus was on using as little ESP32 resources as possible.

Motivation for writing it was the missing capability of my ESP32-Radio (based on Ed Smallenburg's great project) to play audio content stored on my different NAS devices (running a DLNA server).

The library has been tested so far with the following DLNA media servers: 

  - DiXim and Twonky running on Linux (NAS devices) and 
  - UMS (Universal Media Server) & Windows Media Player running on Win10 (in a virtual machine)
	
If you run into trouble with your particular DLNA media server or NAS, increase CORE_DEBUG_LEVEL and it gives you an indication where the problem is. Tracing the communication with Wireshark helps a lot!

## Using W5x00 Ethernet shield/boards instead of builtin WiFi

Using a Wiznet W5500 board and the standard Arduino Ethernet lib for communication produced some sporadic issues. Especially client.read() calls returned corrupted data every now and then, esp. with other threads using the SPI bus simultaneously.
This problem is mentioned a few times in forums on the internet, so it seems to be a ESP32 Arduino SPI library issue.

The only remedy I found was to wrap all function calls within a project that use SPI with a global mutex lock (claimSPI/releaseSPI). This completely wiped all those problems and none of these garbling issues happened ever again. 
See example UsingMutexLocks_Ethernet for more info.

Of course, the ESP32 Arduino SPI library already uses locks (SPI_MUTEX_LOCK/_UNLOCK) but doesn't seem to be 100% thread proof though. Please correct me if I'm wrong or you do find a better solution.

## SSDP multicast

SSDP M-SEARCH multicast packets carry a destination ip 239.255.255.250 and destination port 1900.
However, some NAS devices (e.g. Buffalo's NAS LS520D) unexpectedly do NOT reply to such packets when the >>source<< port is 1900 as well. Some articles on the internet state that NAS devices might show this behaviour to prevent SSDP (r)DDoS storms.

Unfortunately when using the standard Arduino Ethernet library (instead of WiFi) all SSDP multicast packets carry identical destination & source ports, so in our case 1900. Therefore my Buffalo NAS LS520D never responded to SSDP search requests when using an Ethernet module/shield.

There are two solutions to this problem:

a) use addServer() to manually add a server to the server list (which I recommend), or

b) modify "socket.cpp" in Arduino Ethernet library to use a >>source<< port between 49152 & 65535. Which is not only a dirty solution but puts you under risk to forget about it and then loose those changes when updating the Ethernet lib a year later in a hurry. Have a look at doc/Readme.txt to see the suggested modification.

## Compiling the examples

If possible, please always set project wide compiler option "__GNU_VISIBLE" which enables usage of strcasestr() provided in "string.h". 
However, if "__GNU_VISIBLE" is not defined, a quick and dirty version of strcasestr() defined in SoapESP32.cpp will be used instead.

If you use an Ethernet module/shield instead of builtin WiFi you must set the compiler option "USE_ETHERNET".

Unfortunately we can't set project wide compiler options in *.ino sketches though. 

In case you use the Arduino IDE:

Add any needed additional compiler options to line 'compiler.cpreprocessor.flags' in your Arduino IDE file "platform.txt". 
On my PC for example I find this file in the following directory:  	C:\Users\tj\AppData\Local\Arduino15\packages\esp32\hardware\esp32\1.0.4.  But be reminded, those options will stay permanent until you delete them. So having "-DUSE_ETHERNET" in platform.txt does NOT work with examples that use WiFi (..._WiFi.ino).  Alternatively you could just uncomment the line //#define USE_ETHERNET in SoapESP32.h. Whatever you prefer.

In case you use VSCode/PlatformIO:

You are lucky. Simply add/remove global compiler options in your platformio.ini project file:

build_flags = -D__GNU_VISIBLE, -DUSE_ETHERNET
	
## Doc 

Folder "doc" contains log files showing Serial Monitor output of different core debug levels of the various examples and of an ESP32-Radio with integrated SoapESP32. They might help you in adding this lib to your project.
		
After merging this library with an existing ESP32-Radio it's running for a few month now without any problems. No need to fiddle around with SD cards anymore.

The lib is not perfect but I hope you might find it useful.

Thomas J. <yellobyte@bluewin.ch>