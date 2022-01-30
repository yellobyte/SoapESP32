# ESP32 Webradio++

This ESP32 internet radio implementation is based on Ed Smallenburgs (ed@smallenburg.nl) original ESP32 Radio project, version 10.06.2018. His still active project is documented at https://github.com/Edzelf/ESP32-radio.

My main reason for making this project **ESP32 Webradio++** public on github is to show  the implementation of the [**SoapESP32**](https://github.com/yellobyte/SoapESP32) library into my own webradio project, which I have been asked for a few times lately. 
The second reason is to provide an example of how to give a second life to an old and outdated device you have used for years and are really unwilling to part with...

SoapESP32 library enables any ESP32 device to connect to DLNA media servers in the local network, browse their content and download selected files.  

Using a rotary switch encoder is all it needs to browse through the content of a DLNA media server in the local network. Going up and down the directory levels and finally selecting an audio file for playing can be done very fast.

In this project, after selecting a file from the list returned by *browseServer()*, we send a read request to the media server using *readStart()* and if granted, *read()* a chunk of data into the queue which feeds the audio codec VS1053B. When the queue accepts another chunk of data we *read()* more from the media server until end of file. Then we close the data connection to the server with *readStop()* and automatically select the next audio file from the browse list, provided we are in repeat file/folder mode.  

## Feature list ##

Starting from Ed's code (Version 10.06.2018) this **ESP32 Webradio++** project has seen a lot of additions and modifications over time. Here a summary:

 * integration of SoapESP32 library
 * Digital audio output added (TOSLINK optical) using a WM8805 module (Aliexpress)
 * Usage of own VS1053 decoder board (with I2S output and w/o 3.5mm audio sockets)<br />
   -> You find the Eagle schematic & board files [here](https://github.com/yellobyte/ESP32-Webradio-PlusDLNA/blob/main/EagleFiles).
 * VS1053 gets patched with new firmware v2.7 at each reboot<br />
   -> Latest firmware patches for the VLSI VS1053 are available from [here](http://www.vlsi.fi/en/support/software/vs10xxpatches.html).
 * VU meter added on TFT display (needs above mentioned firmware patch)
 * Debug Server on port 23 for debug output (when compiled in than the cmd server is omitted)
 * Handling of original TECHNICS Tuner ST-G570 front panel buttons & LEDs via I2C and extender module
 * SD card indexing code rewritten in large parts
 * Usage of W5500 Ethernet module instead of builtin WiFi
 * Ability to update ESP32 Radio code via SD card (using OTA functionality during reboot)
 * Encoder debouncing done completely in hardware (RC + Schmitt-Trigger IC), we use a Stec Rotary Encoder STEC11B03 with 1 impulse per 2 clicks
 * MP3 progress bar while playing audio
 * MQTT functionality & battery stuff removed completely
 * handling of more special chars in webradio streams (some channels seems to have a different utf8 conversion table or philosophy)
 * countless minor changes, some bugfixing

If interested, have a look at "Revision history.txt" in the doc folder. 

### Still to be done (just an idea):

 * Enable the Webinterface to browse DLNA media servers and select files

## Implementation of SoapESP32 library ##

The following sample picture sequence shows the actual implementation into this webradio project:

![github](https://github.com/yellobyte/SoapESP32/raw/main/doc/ESP32-Radio-DLNA.jpg)

Alternatively have a look at the short clip _ESP32-Radio-DLNA.mp4_ in folder **Doc** to see the final implementation in action. To watch now, click [Here](https://github.com/yellobyte/ESP32-Webradio-PlusDLNA/blob/main/Doc/ESP32-Radio-DLNA.mp4).