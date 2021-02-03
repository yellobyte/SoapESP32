All examples were compiled & tested with ArduinoIDE V1.8.9 and VSCode V1.52.1/PlatformIO Core5.0.4/Home 3.3.1.

The following additional files might help you to integrate SoapESP32 lib into your project and support you 
in case of problems during development.

Folder "Logfiles" contains serial monitor output of most examples, some with different CORE_DEBUG_LEVEL settings.

Folder "Hardware" shows schematic & wiring used for all tests with Ethernet & Micro-SD.

File "platformio.ini" was used while developing with VSCode/PlatformIO (just in case you might wonder).

The snapshot "Using VLC to find download parameter.JPG" shows you how to use VLC to find proper download 
parameters needed in examples "DownloadFileExample1_xxxxx.ino".

ESP32-Radio:
If you want to integrate this library into an existing ESP32-Radio project, please have a look at the short 
mp4 clip and the corresponding logfile. They might be helpful.


The proposed modification for socket.cpp as mentioned in Readme.md to get around the SSDP problem is as follows:

uint8_t EthernetClass::socketBeginMulticast(uint8_t protocol, IPAddress ip, uint16_t port)
{
  ...
  ...
  if (port > 0 && (protocol != (SnMR::UDP | SnMR::MULTI))) {	<------ modification !!
    W5100.writeSnPORT(s, port);
  } else {
    // if don't set the source port, set local_port number.
    if (++local_port < 49152) local_port = 49152;
    W5100.writeSnPORT(s, local_port);
  }
  // Calculate MAC address from Multicast IP Address
  byte mac[] = {  0x01, 0x00, 0x5E, 0x00, 0x00, 0x00 };
  ...
  ...



