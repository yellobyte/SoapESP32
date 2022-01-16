# SoapESP32

This Arduino library provides basic UPnP/SOAP functionality, enabling an ESP32 device to scan the local network for DLNA media servers, browse their content and finally download files.

Motivation for writing it was the missing capability of an existing ESP32-Radio (based on Ed Smallenburg's code) to play audio content stored on the various NAS devices in the local network at home (most of them running a DLNA server).

The library has been successfully tested so far with the following DLNA media servers: 

  - **DiXim** and **Twonky** running on Linux (NAS devices) and 
  - **UMS** (Universal Media Server), **Jellyfin**, **Kodi**, **Serviio** and **Windows Media Player** running on Win10 (in a virtual machine)
	
If you run into trouble with your particular DLNA media server or NAS, increase `CORE_DEBUG_LEVEL` and it gives you an indication where the problem is. Tracing the communication with Wireshark helps a lot.

## Application notes

Make sure you have the latest version of Arduino core for ESP32 installed. Older versions might produce build errors with some examples.

In library Version 1.1.0 the struct *soapObject_t* has seen two modifications:  
Firstly, the type of member variable *size* has changed from size_t to uint64_t. If used in your projects, you might have to adjust some code.  
Secondly, a new member variable *sizeMissing* (boolean) has been added. 

All the various DLNA media servers I tested the library with showed some oddities. However, I fixed all compatibility issues I ran across. Please note the following:

- Streams/podcasts: Some media servers (e.g. Fritzbox, Serviio) do **not** provide a size for items (media content) located in their Web/Online/InternetRadio folders. Thanks to Github user KiloOscarRomeo for drawing my attention to this fact. In contrast, UMS (Universal Media Server) always provides a fixed size=9223372034707292159 for items in directory Web (incl. subdirectories Radio, Podcasts, etc.). 

- Empty files: Some media servers (MS MediaPlayer/Kodi/Jellyfin) show empty files, others don't. However, this library by default ignores files with reported size zero. They will not show up in browse results. You can change this behaviour with build option `SHOW_EMPTY_FILES`.

- Empty directories: They always show up in browse results.

- Missing attribute size: Media servers often show items without telling their size. That applies to all item types: streams, video files, audio files, image files, etc. In this case the library will return them in browse results with size=0 and sizeMissing=true. 

- Downloading big files/reading streams: Files with reported size bigger than 4.2GB (SIZE_MAX) will be shown in browse results but an attempt to download them with readStart()/read()/readEnd() will fail. If you want to download them or read endless streams you will have to do it outside this library in your own code.

### Using W5x00 Ethernet shield/boards instead of builtin WiFi (optional)

Using a Wiznet W5x00 board and the standard Arduino Ethernet lib for communication produced some sporadic issues. Especially client.read() calls returned corrupted data every now and then, esp. with other threads using the SPI bus simultaneously.
This problem is mentioned a few times in forums on the internet, so it seems to be a known ESP32 Arduino SPI library issue.

The only remedy I found was to wrap all function calls that use SPI with a global/project wide mutex lock (realized within this library with the aid of claimSPI()/releaseSPI()). This completely wiped all those problems and none of the garbling issues happened ever again. See example UsingMutexLocks_Ethernet.ino for more details.

Of course, the ESP32 Arduino SPI library already uses locks (SPI_MUTEX_LOCK/SPI_MUTEX_UNLOCK) but doesn't seem to be 100% thread proof though. Please correct me if I'm wrong or you do find a better solution.

### SSDP multicast issue:

SSDP M-SEARCH multicast packets carry a destination ip 239.255.255.250 / port 1900. However, some NAS devices (e.g. Buffalo's NAS LS520D) unexpectedly do NOT reply to such packets when the **source** port is 1900 as well. Some articles on the internet state that NAS devices might show this behaviour to prevent SSDP (r)DDoS storms.

Unfortunately when using the standard Arduino Ethernet library (instead of WiFi) all SSDP multicast packets carry identical destination & source ports, so in our case 1900. Therefore my Buffalo NAS LS520D never responded to SSDP search requests when using an Ethernet module/shield.

*There are two solutions to this problem:*

a) use addServer() to manually add a server to the server list (which I recommend) **or**

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

### Setting compiler/build options:

All examples were build & tested with ArduinoIDE V1.8.9 and VSCode/PlatformIO.

If possible, please always set project wide preprocessor option `__GNU_VISIBLE` which enables usage of strcasestr() provided in _string.h_. 
However, if `__GNU_VISIBLE` is not defined, a quick and dirty version of strcasestr() defined in _SoapESP32.cpp_ will be used instead.

If you use an Ethernet module/shield instead of builtin WiFi you **must** set the preprocessor option `USE_ETHERNET`. This option must **not** be set when compiling examples that use WiFi (..._WiFi.ino).

### Arduino IDE:

Unfortunately we can't set project wide build options in *.ino sketches. So the easiest way is to uncomment the line **//#define USE_ETHERNET** in _SoapESP32.h_.

Alternatively you could add any needed build options to line **compiler.cpreprocessor.flags** in your Arduino IDE file _platform.txt_.  On my PC for example I find this file in directory:  	
* _C:\Users\tj\AppData\Local\Arduino15\packages\esp32\hardware\esp32\1.0.4_

Be reminded, those options will stay permanent until you delete them!  


### VSCode/PlatformIO:

Simply add/remove build options to your _platformio.ini_ project file as required, e.g.:

build_flags = `-D__GNU_VISIBLE, -DSHOW_EMPTY_FILES, -DUSE_ETHERNET`
	
## How to find correct server parameters needed in some examples

Four of the examples, namely 
* _BrowseRecursively_Ethernet.ino_
* _BrowseRecursively_WiFi.ino_
* _DownloadFileExample1_Ethernet.ino_
* _DownloadFileExample1_WiFi.ino_

require some parameters (that apply to your specific DLNA media server) be defined manually.  

The three snapshots _Using_VLC_to_find_....._parameter.JPG_ in folder **Doc** show you how to use the program **VLC** to find proper values.

## Documentation

Folder **Doc** contains a big collection of files to help you implement this library into your projects:
* the platformio.ini file I used when testing examples with VSCode/PlatformIO
* log files (Serial Monitor output) of all examples, mostly with different core debug levels
* Schematic diagram (wiring) plus picture of the test set used
* VLC snapshots to help you find server & file download parameters needed for some examples

## Example of an implementation: ESP32-Radio using SoapESP32

After merging this library with the ESP32-Radio code it has been running for a few month now without any problems. Biggest advantage is not having to fiddle around with SD cards anymore!

Using the rotary switch encoder is all it needs to browse through the content of a media server in the local network. Going up and down the directory levels and finally selecting an audio file for playing can be done very fast.

Instead of downloading the file to SD card (as explained in examples) we only read a chunk of data into the queue which feeds the audio codec VS1053B. When the queue accepts another chunk of data we read more from the media server until the song has finished. Then we close the data connection to the server and select the next audio file from the list, provided we are in repeat file/folder mode.  

The following picture sequence shows how it is implemented:

![github](https://github.com/yellobyte/SoapESP32/raw/main/doc/ESP32-Radio-DLNA.jpg)

Alternatively have a look at the short clip _ESP32-Radio-DLNA.mp4_ in folder **Doc** to see the final implementation in action. To watch now, click [Here](https://github.com/yellobyte/SoapESP32/raw/main/doc/ESP32-Radio-DLNA.mp4)