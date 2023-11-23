# SoapESP32

This Arduino library provides basic UPnP/SOAP functionality, enabling an ESP32 device to scan the local network for DLNA media servers, browse their content and finally download files.

The library has been successfully tested so far with the following DLNA media servers: **DiXim**, **Twonky**, **UMS** (Universal Media Server), **Jellyfin**, **Emby**, **Kodi**, **Plex**, **Serviio**, **Subsonic**, **MinimServer**, **QNAP-DLNA**, **Mezzmo** and **Windows Media Player**.

Integrating this library into your ESP32 Arduino projects is easy. For detailed infos have a look at the many [examples](https://github.com/yellobyte/soapESP32/blob/main/examples) included. Below the basics for searching and printing media servers in your local network. 
```c
...
#include "SoapESP32.h"

WiFiClient client;
WiFiUDP    udp;

SoapESP32    soap(&client, &udp);
soapServer_t srv;
uint8_t      srvNum = 0;

setup() {
  // setting up Wifi, serial output, etc.
  ...
  // scan the local network for media servers (default scan duration is 60 sec)
  // - passing an integer value 5...120 to seekServer() changes the scan duration, 
  //   e.g. soap.seekServer(10) would scan the network for only 10 sec
  // - please note: longer scan times increase the chance for detecting media servers
  soap.seekServer();
  // printing details of all discovered servers
  while (soap.getServerInfo(srvNum++, &srv)) {
    Serial.print("Server name: ");
    Serial.println(srv.friendlyName);
    ...
  }
}
...
```

## :zap: Application notes

To install the library into your **IDE** open the **Library Manager**, search for **SoapESP32** and choose **install**, **Add to Project** or similar.  

Always make sure you have one of the latest versions of **Arduino core for ESP32** installed. Older versions might produce build errors with some examples.

Most DLNA media servers I tested the library with showed some oddities. All compatibility issues I ran across have been fixed. Please note the following:

- As of V1.3.0 a new function _searchServer()_ is available. The function sends UPnP search requests to media servers asking for a list of items that match certain criterias, e.g. the items _title_ must contain the string "xyz" or the files property _album_ must contain the string "abc", etc. Not all media servers support UPnP search requests though. More info below.

- Some media servers do not answer SSDP M-SEARCH requests but instead broadcast NOTIFY messages regularly, e.g. every minute or less. Therefore as of V1.2.0 the default network scan time of function _seekServer()_ has been increased from 5s to 60s, the scan section of this function has been improved and the function now accepts an integer value (5...120) for setting a specific scan duration (in sec) if needed.

- Streams/podcasts: Some media servers (e.g. Fritzbox, Serviio) do **not** provide a size for items (media content) located in their Web/Online/InternetRadio folders. Thanks to Github user KiloOscarRomeo for drawing my attention to this fact. In contrast, UMS (Universal Media Server) always provides a fixed size of 9223372034707292159 (0x7FFFFFFF7FFFFFFF) for items in directory Web (incl. subdirectories Radio, Podcasts, etc.). 

- Empty files: Some media servers (MS MediaPlayer/Kodi/Jellyfin) show empty files, others don't. This library by default ignores files with reported size zero. They will not show up in browse results. You can change this behaviour with build option `SHOW_EMPTY_FILES`.

- Empty directories: They always show up in browse results.

- Missing attribute size: Media servers often show items without telling their size. That applies to all item types: streams, video/audio/image files, etc. In this case the library will return them in browse results with *size=0* and *sizeMissing=true*. 

- Missing attributes "size" (means child count in case of directories) & "searchable" (for directories): Some servers report containers (directories) with size=0 or without this attribute when in fact they are not empty. Same applies to attribute searchable. This is very annoying for it forces you to dig into each (sub)directory not to overlook anything.

- Parent-ID Mismatch: The id of a directory and the parent id of it's content should match. Sometimes it does not (e.g. Subsonic). As of V1.1.1 this mismatch is ignored by default. You can go back to strict behaviour with build option `PARENT_ID_MUST_MATCH`

- Downloading big files/reading streams: Files with reported size bigger than 4.2GB (SIZE_MAX) will be shown in browse results but an attempt to download them with *readStart()/read()/readEnd()* will fail. If you want to download them or read endless streams you will have to do it outside this library in your own code.

- IP & port for file download can be different from the media server's IP & port! So always evaluate *downloadIp* & *downloadPort* in media server objects returned by *browseServer()* when a download is intended.
	
If you run into trouble with your particular DLNA media server or NAS, increase `CORE_DEBUG_LEVEL` and it gives you an indication where the problem is. Tracing the communication with Wireshark can help as well.

### :mag: Searching for files using UPnP search requests

The doc files and/or manuals of almost all media servers give no info as to a servers UPnP search capabilities. Hence it was a typical trial-and-error approach.

Of all the media servers I tested only **Twonky**, **Emby**, **Mezzmo** and **MinimServer** accepted search requests to a various extent. This of course might depend on the version used (free version or fully licensed) as well as the server's software release, etc.  

Below follow some examples for successfully tested **search criterias**:
1) Searching for items whose property **title** contains the string "wind":  
   **dc:title contains "wind"**  
All the following titles would match above criteria: "Wind", "Winds of change", "slow winds", "rewind", etc.
2) Searching for files whose property **album** contains the string "Best Of":  
   **upnp:album contains "Best Of"**
3) Searching for files whose property **artist** contains the string "John":  
   **upnp:artist contains "John"**
4) Searching for files whose property **genre** contains the string "New Age" (did not work with Mezzmo):  
   **"upnp:genre contains "New Age"**
5) Even combined search criterias were accepted (did not work with Mezzmo), e.g.:  
   **upnp:genre contains "New Age" and dc:title contains "March"**
6) Only with Twonky searching for a class of files was possibly, e.g. for **video** files:  
   **upnp:class derivedfrom "object.item.videoItem"**
7) And combinations like **video file** and **title**:  
   **upnp:class derivedfrom "object.item.videoItem" and dc:title contains "street"**
8) Or **audio file** and **album**:  
   **upnp:class derivedfrom "object.item.audioItem" and dc:album contains "One"**
9) Or **album folder** and **title**:  
   **upnp:class derivedfrom "object.container.album" and dc:title contains "Songs"**

Twonky accepted optional **sort criterias**. They define the sort order of the items returned (if any). Successfully tested sort criterias were:
1) Name of title, ascending (default) --> sort criteria: **"+dc:title"**
2) Name of title, descending --> sort criteria: **"-dc:title"**

I don't claim above list of search/sort criterias to be complete. However, those were the ones I needed and I haven't bothered to look for more.

The provided example sketches _SearchServerExample.....ino_ and the accompanying log files _SearchServerExample...log_ demonstrate the usage of the search function **_searchServer()_**. The function simply sends a search request (containing up to two search criterias) to the media server and waits for the server to reply with a list of matching items. 

Just to give you a better idea, running example _SearchServerExample1_WiFi.ino_ in my home network and searching for all files/folders whose title contains the string "words" (search criteria: **dc:title contains "words"**) produced the following slightly stripped-down result:
```c
21:48:49.384 > Connecting to WiFi network ..
21:48:54.201 > Connected successfully. IP address: 192.168.1.46
21:48:54.201 >
21:48:54.201 > Scanning local network for DLNA media servers...
21:49:49.370 > Number of discovered servers that deliver content: 2
21:49:49.370 >
21:49:49.370 > Server: Twonky [QNAP-TS253D]
21:49:50.721 > Search results: 10
21:49:50.721 >  Words (Between the Lines of Age) (6:40) (Item, size: 12819052, artist: Neil Young)
21:49:50.721 >  Do I Have to Say the Words? (Item, size: 14939806, artist: Bryan Adams)
...
...
21:49:50.789 >
21:49:50.789 > Server: MinimServer[QNAP-TS253D]
21:49:51.253 > Search results: 7
21:49:51.269 >  A blank silence greeted Hermione's words. (Item, size: 1079928, artist: J. K. Rowling)
21:49:51.269 >  A gale of laughter from the middle of the table drowned the rest of Bill's words. (Item, size: 1165231, artist: J. K. Rowling)
...
...
```
Hint: Similar to function browseServer(), if the returned list contains 100 (SOAP_DEFAULT_MAX_COUNT) items you might try again with increasing starting index 100, 200, 300 and so on to get all items matching the criteria(s).

### :heavy_exclamation_mark: Using W5x00 Ethernet shield/boards instead of builtin WiFi (optional)

Using a Wiznet W5x00 board and the standard Arduino Ethernet lib for communication produced some sporadic issues. Especially client.read() calls returned corrupted data every now and then, esp. with other threads using the SPI bus simultaneously.
This problem is mentioned a few times in forums on the internet, so it seems to be a known ESP32 Arduino SPI library issue.

The only remedy I found was to wrap all function calls that use SPI with a global/project wide mutex lock (realized within this library with the aid of *claimSPI()/releaseSPI()*). This completely wiped all those problems. See example [*UsingMutexLocks_Ethernet.ino*](https://github.com/yellobyte/SoapESP32/tree/main/examples/UsingMutexLocks_Ethernet/UsingMutexLocks_Ethernet.ino) for more details.

Of course, the ESP32 Arduino SPI library already uses locks (SPI_MUTEX_LOCK/SPI_MUTEX_UNLOCK) but doesn't seem to be 100% thread proof though. Please correct me if I'm wrong or you do find a better solution.

### :hammer_and_wrench: Setting compiler/build options:

All examples were build & tested with various versions of ArduinoIDE and VSCode/PlatformIO.

If preprocessor option `__GNU_VISIBLE` is already defined then strcasestr() provided by toolchain is used, if not then its equivalent from _SoapESP32.cpp_ will be used.

If you use an Ethernet module/shield instead of builtin WiFi you must set the preprocessor option `USE_ETHERNET`. Otherwise the build will fail.

#### Building with Arduino IDE:

Add a file named **_build_opt.h_** containing your wanted build options to your sketch directory, e.g.:  
```c
-DUSE_ETHERNET
-DSHOW_ESP32_MEMORY_STATISTICS
```
**Please note:** Changes made to _build_opt.h_ after a first build will not be detected by the Arduino IDE. Rebuilding the whole project or restarting the IDE will fix that.  

#### Building with VSCode/PlatformIO:

Add wanted build options to your project file **_platformio.ini_** , e.g.:  
```c
build_flags = -DUSE_ETHERNET
```

## :mag: How to find correct server parameters needed in some examples

Four examples require parameters being set that apply to your specific DLNA media server:
- _BrowseRecursively_Ethernet.ino_
- _BrowseRecursively_WiFi.ino_
- _DownloadFileExample1_Ethernet.ino_
- _DownloadFileExample1_WiFi.ino_

The open source media player **VLC** makes it easy to find the right values. Three snapshots in folder [**Doc**](https://github.com/yellobyte/soapESP32/blob/main/doc) show you how to find them.

## :file_folder: Documentation

Folder [**Doc**](https://github.com/yellobyte/soapESP32/blob/main/doc) contains a big collection of files to help you implement the library into your projects:
* The platformio.ini file I used when testing examples with VSCode/PlatformIO
* A build log
* Plenty of log files (serial monitor output) of all examples, mostly with different core debug levels
* Schematic diagram (wiring) plus picture of the test set used
* VLC snapshots to help you find the right server parameters needed for certain examples

## :tada: Example of an implementation: ESP32-Radio utilizing SoapESP32

An example of a real world implementation can be found here: [ESP32-Radio project](https://github.com/yellobyte/ESP32-Webradio-PlusDLNA). No fiddling around with SD cards anymore. Using the rotary switch encoder is all it needs to browse through the content of a DLNA media server in the local network and select an audio file. After selecting the file, the radio simply reads the data from the server and writes it into the queue which feeds the audio codec VS1053B.  

![github](https://github.com/yellobyte/SoapESP32/raw/main/doc/ESP32-Radio-DLNA.jpg)

Alternatively have a look at the short clip _ESP32-Radio-DLNA.mp4_ in folder **Doc** to see the final implementation in action. To watch now, click [Here](https://github.com/yellobyte/SoapESP32/raw/main/doc/ESP32-Radio-DLNA.mp4)

## :relaxed: Postscript

If you run into trouble with your mediaserver or have suggestions how to improve the lib, feel free to contact me or create an issue.  

