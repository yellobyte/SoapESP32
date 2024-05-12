/*
  SoapESP32, a simple library for accessing DLNA media servers with ESP32 devices
	
  Copyright (c) 2021 Thomas Jentzsch

  Permission is hereby granted, free of charge, to any person
  obtaining a copy of this software and associated documentation 
  files (the "Software"), to deal in the Software without restriction, 
  including without limitation the rights to use, copy, modify, merge, 
  publish, distribute, sublicense, and/or sell copies of the Software, 
  and to permit persons to whom the Software is furnished to do so, 
  subject to the following conditions:

  The above copyright notice and this permission notice shall be 
  included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR 
  ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF 
  CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION 
  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "SoapESP32.h"
#include "MiniXPath.h"

#ifdef USE_ETHERNET
// usage of Wiznet W5x00 Ethernet board/shield instead of builtin WiFi
#define claimSPI()   if (m_SPIsem && *m_SPIsem) while (xSemaphoreTake(*m_SPIsem, 10) != pdTRUE)
#define releaseSPI() if (m_SPIsem && *m_SPIsem) xSemaphoreGive(*m_SPIsem)
#else
// usage of builtin WiFi
#define claimSPI()
#define releaseSPI() 
#endif

enum eXpath { xpFriendlyName = 0, xpServiceType, xpControlUrl, 
              xpBrowseContainer, xpBrowseContainerAlt1, xpBrowseContainerAlt2,
              xpBrowseItem, xpBrowseItemAlt1, xpBrowseItemAlt2, 
              xpBrowseNumberReturned, xpBrowseNumberReturnedAlt1, xpBrowseNumberReturnedAlt2,
              xpSearchContainer, xpSearchContainerAlt1, xpSearchContainerAlt2,
              xpSearchItem, xpSearchItemAlt1, xpSearchItemAlt2, 
              xpSearchNumberReturned, xpSearchNumberReturnedAlt1, xpSearchNumberReturnedAlt2,
              xpGetSearchCapabilities, xpGetSearchCapabilitiesAlt1, xpGetSearchCapabilitiesAlt2,
              xpGetSortCapabilities, xpGetSortCapabilitiesAlt1, xpGetSortCapabilitiesAlt2,
              xpTitle, xpAlbum, xpArtist, xpGenre, xpClass, xpResource };

xPathParser_t xmlParserPaths[] = { 
  { .num = 3, .tagNames = { "root", "device", "friendlyName" } },
  { .num = 5, .tagNames = { "root", "device", "serviceList", "service", "serviceType" } },
  { .num = 5, .tagNames = { "root", "device", "serviceList", "service", "controlURL" } },
  { .num = 6, .tagNames = { "s:Envelope", "s:Body", "u:BrowseResponse", "Result", "DIDL-Lite", "container" } },
  { .num = 6, .tagNames = { "SOAP-ENV:Envelope", "SOAP-ENV:Body", "m:BrowseResponse", "Result", "DIDL-Lite", "container" } },
  { .num = 6, .tagNames = { "SOAP-ENV:Envelope", "SOAP-ENV:Body", "u:BrowseResponse", "Result", "DIDL-Lite", "container" } },
  { .num = 6, .tagNames = { "s:Envelope", "s:Body", "u:BrowseResponse", "Result", "DIDL-Lite", "item" } },
  { .num = 6, .tagNames = { "SOAP-ENV:Envelope", "SOAP-ENV:Body", "m:BrowseResponse", "Result", "DIDL-Lite", "item" } },
  { .num = 6, .tagNames = { "SOAP-ENV:Envelope", "SOAP-ENV:Body", "u:BrowseResponse", "Result", "DIDL-Lite", "item" } },
  { .num = 4, .tagNames = { "s:Envelope", "s:Body", "u:BrowseResponse", "NumberReturned" } },
  { .num = 4, .tagNames = { "SOAP-ENV:Envelope", "SOAP-ENV:Body", "m:BrowseResponse", "NumberReturned" } },
  { .num = 4, .tagNames = { "SOAP-ENV:Envelope", "SOAP-ENV:Body", "u:BrowseResponse", "NumberReturned" } },
  { .num = 6, .tagNames = { "s:Envelope", "s:Body", "u:SearchResponse", "Result", "DIDL-Lite", "container" } },
  { .num = 6, .tagNames = { "SOAP-ENV:Envelope", "SOAP-ENV:Body", "m:SearchResponse", "Result", "DIDL-Lite", "container" } },
  { .num = 6, .tagNames = { "SOAP-ENV:Envelope", "SOAP-ENV:Body", "u:SearchResponse", "Result", "DIDL-Lite", "container" } },  
  { .num = 6, .tagNames = { "s:Envelope", "s:Body", "u:SearchResponse", "Result", "DIDL-Lite", "item" } },
  { .num = 6, .tagNames = { "SOAP-ENV:Envelope", "SOAP-ENV:Body", "m:SearchResponse", "Result", "DIDL-Lite", "item" } },
  { .num = 6, .tagNames = { "SOAP-ENV:Envelope", "SOAP-ENV:Body", "u:SearchResponse", "Result", "DIDL-Lite", "item" } },  
  { .num = 4, .tagNames = { "s:Envelope", "s:Body", "u:SearchResponse", "NumberReturned" } },
  { .num = 4, .tagNames = { "SOAP-ENV:Envelope", "SOAP-ENV:Body", "m:SearchResponse", "NumberReturned" } },
  { .num = 4, .tagNames = { "SOAP-ENV:Envelope", "SOAP-ENV:Body", "u:SearchResponse", "NumberReturned" } },  
  { .num = 4, .tagNames = { "s:Envelope", "s:Body", "u:GetSearchCapabilitiesResponse", "SearchCaps" } },
  { .num = 4, .tagNames = { "SOAP-ENV:Envelope", "SOAP-ENV:Body", "m:GetSearchCapabilitiesResponse", "SearchCaps" } },
  { .num = 4, .tagNames = { "SOAP-ENV:Envelope", "SOAP-ENV:Body", "u:GetSearchCapabilitiesResponse", "SearchCaps" } },  
  { .num = 4, .tagNames = { "s:Envelope", "s:Body", "u:GetSortCapabilitiesResponse", "SortCaps" } },
  { .num = 4, .tagNames = { "SOAP-ENV:Envelope", "SOAP-ENV:Body", "m:GetSortCapabilitiesResponse", "SortCaps" } },
  { .num = 4, .tagNames = { "SOAP-ENV:Envelope", "SOAP-ENV:Body", "u:GetSortCapabilitiesResponse", "SortCaps" } },  
  { .num = 1, .tagNames = { "dc:title" } },
  { .num = 1, .tagNames = { "upnp:album" } },
  { .num = 1, .tagNames = { "upnp:artist" } },
  { .num = 1, .tagNames = { "upnp:genre" } },
  { .num = 1, .tagNames = { "upnp:class" } },
  { .num = 1, .tagNames = { "res" } }
};

const char *fileTypes[] = { "other", "audio", "picture", "video", "" };

//
// helper function, find the first occurrence of substring "what" in string "s", ignore case
//
#if !defined(__GNU_VISIBLE)
char *strcasestr(const char *s, const char *what)
{
  char c, sc;
  size_t len;

  if ((c = *what++) != 0) {
    c = tolower((unsigned char)c);
    len = strlen(what);
    do {
      do {
        if ((sc = *s++) == 0) return (NULL);
      } 
      while ((char)tolower((unsigned char)sc) != c);
    } 
    while (strncasecmp(s, what, len) != 0);
    s--;
  }

  return ((char *)s);
}
#endif

//
// SoapESP32 Class Constructor
//
#ifdef USE_ETHERNET
SoapESP32::SoapESP32(EthernetClient *client, EthernetUDP *udp, SemaphoreHandle_t *sem)
  : m_client(client), m_udp(udp), m_SPIsem(sem), m_clientDataConOpen(false), m_clientDataAvailable(0)
#else
SoapESP32::SoapESP32(WiFiClient *client, WiFiUDP *udp)
  : m_client(client), m_udp(udp), m_clientDataConOpen(false), m_clientDataAvailable(0)
#endif
{
}

//
// broadcast 3 WOL packets carrying a specified MAC address
// - parameter is a pointer to a C string in the format "00:1:23:Aa:bC:D4" as an unusual example
//
#define WOL_PACKET_SIZE 102
bool SoapESP32::wakeUpServer(const char *macAddress)
{
  uint8_t packetBuffer[WOL_PACKET_SIZE];
  int mac[6];
  char lower[20];

  if (!m_udp) return false;

  if (strlen(macAddress) > 10 && strlen(macAddress) < 18) {
    int i;
    for (i = 0; i < strlen(macAddress); i++) {
      lower[i] = tolower(macAddress[i]);
    }
    lower[i] = 0;
    if (sscanf(lower, "%x:%x:%x:%x:%x:%x%*c",
               &mac[0], &mac[1], &mac[2],
               &mac[3], &mac[4], &mac[5]) != 6) {
      log_e("could not parse parameter MAC \"%s\"", macAddress);
      return false;
    }
    // build WOL udp packet: 6 bytes 0xFF followed by 16 x mac of target device
    memset(packetBuffer, 0xFF, sizeof(packetBuffer));
    for (i = 6; i < WOL_PACKET_SIZE; i++) {
      packetBuffer[i] = (uint8_t)*(mac + (i % 6));
    }
    // broadcast 3 WOL packets (destination IP 255.255.255.255, port 9)
    for (i = 0; i < 3; i++) {
      claimSPI();
      m_udp->begin(9);
      int ret = m_udp->beginPacket(IPAddress(255,255,255,255), 9);
      releaseSPI();
      if (ret) {
        claimSPI();
        m_udp->write(packetBuffer, WOL_PACKET_SIZE);
        m_udp->endPacket();
        m_udp->stop();
        releaseSPI();
        continue;
      }
      break;
    }
    if (i == 3) return true;
  }
  else {
    log_e("parameter MAC is invalid");
  }

  return false;
}

//
// helper function, client timed read
//
int SoapESP32::soapClientTimedRead(unsigned long ms)
{
  int c;
  unsigned long timeout = ms ? ms : SERVER_READ_TIMEOUT,
                startMillis = millis();

  do {
    claimSPI();
    c = m_client->read();
    releaseSPI();
    if (c >= 0) {
      return c;
    }
  } 
  while (millis() - startMillis < timeout);

  return -1;     // read timeout
}

//
// send SSDP/UDP multicast M-SEARCH packets
// - parameter is how often a M-SEARCH packet is to be repeated
//
bool SoapESP32::soapUDPmulticast(unsigned int repeats)
{
  if (!m_udp) return false;

#ifdef USE_ETHERNET
  claimSPI();
  uint8_t ret = m_udp->beginMulticast(IPAddress(SSDP_MULTICAST_IP), SSDP_MULTICAST_PORT);      // sets multicast dest port 
  releaseSPI();
#else
  uint8_t ret = m_udp->beginMulticast(IPAddress(SSDP_MULTICAST_IP), SSDP_MULTICAST_PORT);      // sets local port
#endif  
  if (ret) {
    // creating UDP socket ok
    unsigned int i = 0;
    String strMS(SSDP_M_SEARCH), strCD(SSDP_M_SEARCH);

    strMS += SSDP_DEVICE_TYPE_MS; strMS += "\r\n\r\n";
    strCD += SSDP_SERVICE_TYPE_CD; strCD += "\r\n\r\n";

    // send M-SEARCH packets with device type MediaServer & service type ContentDirectory
    while (true) {
      claimSPI();
      if (!m_udp->beginPacket(IPAddress(SSDP_MULTICAST_IP), SSDP_MULTICAST_PORT) ||
          !m_udp->write((const uint8_t*)strMS.c_str(), (size_t)strMS.length()) ||
          !m_udp->endPacket()) {
        releaseSPI();
        break;
      }
      releaseSPI();
      log_v("SSDP M-SEARCH packet sent:\n%s", strMS.c_str());
      claimSPI();
      if (!m_udp->beginPacket(IPAddress(SSDP_MULTICAST_IP), SSDP_MULTICAST_PORT) ||
          !m_udp->write((const uint8_t*)strCD.c_str(), (size_t)strCD.length()) ||
          !m_udp->endPacket()) {
        releaseSPI();
        break;
      }
      releaseSPI();      
      log_v("SSDP M-SEARCH packet sent:\n%s", strCD.c_str());
      if (++i > repeats) {
        return true;
      }      
    }
  }
  claimSPI();  
  m_udp->stop();
  releaseSPI();
  log_e("error sending SSDP M-SEARCH multicast packets");

  return false; 
}

//
// SSDP/UDP search for media servers in local network
//
bool SoapESP32::soapSSDPquery(soapServerVect_t *result, int msWait)
{
  int port;
  unsigned int i;
  size_t len;
  IPAddress ip;
  char location[SSDP_LOCATION_BUF_SIZE] = "",
       address[20];

  // send SSDP multicast packets (parameter: nr of repeats)
  if (!soapUDPmulticast(SSDP_M_SEARCH_REPEATS)) return false;

  // evaluate incoming SSDP packets (M-SEARCH replies) & NOTIFY packets if we catch them by chance
  uint32_t start = millis();
  do
  {
    delay(1);
    claimSPI();
    len = m_udp->parsePacket();
    releaseSPI();
    if (len) {
      char *p, *buffer;
      
      // SSDP packet of size len received
      buffer = (char *)calloc(len + 1, sizeof(byte)); // allocate and set memory to 0
      if (!buffer) {
        claimSPI();  
        m_udp->stop();
        releaseSPI();
        log_e("calloc() couldn't allocate memory");    
        return false;
      }      
      claimSPI();
      m_udp->read(buffer, len);                       // read packet into the buffer
      releaseSPI();
      log_d("SSDP (%s) within %d ms, size %d", strstr(buffer, HTTP_HEADER_200_OK) ? "REPLY" : "NOTIFY", millis() - start, len);
      log_v("packet content:\n%s", buffer);

      // scan SSDP packet
      if ( // M-SEARCH reply packets
           (strstr(buffer, HTTP_HEADER_200_OK) &&
            ((p = strcasestr(buffer, SSDP_LOCATION)) != NULL) && 
             (strcasestr(buffer, SSDP_DEVICE_TYPE_MS) || strcasestr(buffer, SSDP_SERVICE_TYPE_CD))
           ) ||
           // NOTIFY packets sent out regularly by media servers (we ignore ssdp:byebye's)
           (strstr(buffer, SSDP_NOTIFICATION) && strcasestr(buffer, SSDP_NOTIFICATION_SUB_TYPE) &&
            ((p = strcasestr(buffer, SSDP_LOCATION)) != NULL) && 
            (strcasestr(buffer, SSDP_DEVICE_TYPE_MS) || strcasestr(buffer, SSDP_SERVICE_TYPE_CD))
           )
         ) {  
        char format[30];

        strtok(p, "\r\n");
        snprintf(format, sizeof(format), "http://%%[0-9.]:%%d/%%%ds", SSDP_LOCATION_BUF_SIZE - 1);
        if (sscanf(p + 10, format, address, &port, location) < 2) goto CONT;
        if (!ip.fromString(address)) goto CONT;

        // scanning of ip address & port successful, location string can be missing (e.g. D-Link NAS DNS-320L)
        log_d("scanned ip=%s, port=%d, loc=\"%s\"",  ip.toString().c_str(), port, location);
        if (!strlen(location)) log_d("empty location string!");

        // avoid multiple entries of same server (ip & port identic)
        for (i = 0; i < result->size(); i++) {
          if (result->operator[](i).ip == ip && result->operator[](i).port == port) break;               
        }
        if (i < result->size()) goto CONT;

        // new server found: add it to list
        soapServer_t srv = {.ip = ip, .port = (uint16_t)port, .location = location};
        result->push_back(srv);
        log_i("server added to list ip=%s, port=%d, loc=\"%s\"", ip.toString().c_str(), port, location);
      }
CONT:
      free(buffer);
    }
  }
  while ((millis() - start) < msWait);

  claimSPI();
  m_udp->stop();
  releaseSPI();

  return true;
}

//
// read & evaluate HTTP header
//
bool SoapESP32::soapReadHttpHeader(uint64_t *contentLength, bool *chunked)
{
  size_t len;
  bool ok = false;
  char *p, tmpBuffer[TMP_BUFFER_SIZE_200];

  // first line contains status code
  claimSPI();
  len = m_client->readBytesUntil('\n', tmpBuffer, sizeof(tmpBuffer) - 1);   // returns length without terminator '\n'
  releaseSPI();
  tmpBuffer[len] = 0;
  if (!strstr(tmpBuffer, HTTP_HEADER_200_OK)) {
    log_i("header line: %s", tmpBuffer);
    return false;
  }
  else {
    log_v("header line: %s", tmpBuffer);
    // TEST
#if CORE_DEBUG_LEVEL == 5
    delay(1);	// allow pending serial monitor output to be sent
#endif      
  }
  *contentLength = 0;
  if (chunked) *chunked = false;
  while (true) {
    claimSPI();
    int av = m_client->available();
    releaseSPI();
    if (!av) break;
    claimSPI();
    len = m_client->readBytesUntil('\n', tmpBuffer, sizeof(tmpBuffer) - 1);
    releaseSPI();
    tmpBuffer[len] = 0;
    log_v("header line: %s", tmpBuffer);
    if (len == 1) break;      // End of header: finishing line contains only "\r\n"
    if (!ok) {
      if ((p = strcasestr(tmpBuffer, HEADER_CONTENT_LENGTH)) != NULL) {
        if (sscanf(p+strlen(HEADER_CONTENT_LENGTH), "%llu", contentLength) == 1) {
          ok = true;
          continue;           // continue to read rest of header
        }  
      }
      else if (chunked && strcasestr(tmpBuffer, HEADER_TRANS_ENC_CHUNKED)) {
        ok = *chunked = true;
        m_ChunkCount = 0;  // telling chunk size comes next
        continue;             // continue to read rest of header      
      }
    }  
  }
  if (ok) {
    m_xmlReplaceState = xmlPassthrough;
    if (chunked && *chunked) {
      log_d("HTTP-Header ok, trailing content is chunked, no size announced"); 
    }
    else {
      log_d("HTTP-Header ok, trailing content is not chunked, announced size: %llu", *contentLength); 
    }
  }

  return ok;
}

//
// read XML data, de-chunk if needed & replace predefined XML entities that spoil MiniXPath
//
const replaceWith_t replaceWith[] = { {"&lt;", '<'},
                                      {"&gt;", '>'},
                                      {"&quot;", '"'},
                                      {"&amp;amp;", '&'},
                                      {"&amp;apos;", '\''},
                                      {"&amp;quot;", '\''} }; 

int SoapESP32::soapReadXML(bool chunked, bool replace)
{
  bool match;
  int i, c = -10;

  if (!replace || (replace && (m_xmlReplaceState == xmlPassthrough))) {
GET_MORE:    
    if (!chunked) {
      // data is not chunked
      if ((c = soapClientTimedRead()) < 0) {
        // EOF, timeout or connection closed
        return -1;
      }
    }
    else {
      // de-chunk XML data   
      if (m_ChunkCount <= 0) {
        char tmpBuffer[10];
      
        // next line contains chunk size
        claimSPI();
        int len = m_client->readBytesUntil('\n', tmpBuffer, sizeof(tmpBuffer) - 1);
        releaseSPI();
        if (len < 2) {
          return -2;   // we expect at least 1 digit chunk size + '\r'
        }
        tmpBuffer[len-1] = 0;     // replace '\r' with '\0'
        if (sscanf(tmpBuffer, "%x", &m_ChunkCount) != 1) {
          return -3;
        }
        log_d("announced chunk size: 0x%x(%d)", m_ChunkCount, m_ChunkCount);
        if (m_ChunkCount <= 0) {
          return -4;  // not necessarily an error...final chunk size can be 0
        }
      }
      if ((c = soapClientTimedRead()) < 0) {
        return -5;
      }

      // check for end of chunk
      if (--m_ChunkCount == 0) {
        // skip "\r\n" trailing each chunk
        if (soapClientTimedRead() < 0 || soapClientTimedRead() < 0) {
          return -6;   
        }
      }
    }
  }

  // replace predefined XML entities ("&lt;" becomes "<", etc.)
  if (replace) {
    if (m_xmlReplaceState == xmlPassthrough) {
      if (c == '&') {
        memset(m_xmlReplaceBuffer, 0, sizeof(m_xmlReplaceBuffer));
        m_xmlReplaceBuffer[0] = '&';
        m_xmlReplaceState = xmlAmpDetected;
        m_xmlReplaceOffset = 1;
        goto GET_MORE;
      }
    }
    else if (m_xmlReplaceState == xmlAmpDetected) {
      m_xmlReplaceBuffer[m_xmlReplaceOffset++] = c;
      // run through all predefined sequences and see if we still match
      for (match = false, i = 0; i < sizeof(replaceWith)/sizeof(replaceWith_t) && !match; i++) {
        if (strncmp(m_xmlReplaceBuffer, replaceWith[i].replace, m_xmlReplaceOffset) == 0) {
          match = true;
          break;
        }
      }
      if (!match) {
        // single '&' or sequence we don't replace
        c = '&';
        m_xmlReplaceState = xmlTakeFromBuffer;  
        m_xmlReplaceOffset = 1;
      }
      else {  // match
        if (m_xmlReplaceOffset < strlen(replaceWith[i].replace)) {
          goto GET_MORE;
        }
        else {
          // found full sequence to be replaced
          c = replaceWith[i].with;  
          m_xmlReplaceState = xmlPassthrough;
        }
      }
    }
    else {  
      // xmlTakeFromBuffer
      c = m_xmlReplaceBuffer[m_xmlReplaceOffset++];
      if (m_xmlReplaceBuffer[m_xmlReplaceOffset] == '\0') {
        m_xmlReplaceState = xmlPassthrough;
      }
    }
  }
  // TEST
  //Serial.printf("%c", (char)c);
  return c;
}

//
// scanning local network for media servers that offer media content
//  - parameter is scan duration, range 5...120s (without parameter the function defaults to 60s) 
//  - returns number of media servers found
//
unsigned int SoapESP32::seekServer(unsigned int scanDuration)
{
  soapServerVect_t rcvd;

  // delete old server list
  m_server.clear();

  if (scanDuration > 120) scanDuration = 120;
  else if (scanDuration < 5) scanDuration = 5;

  log_i("SSDP search for media servers started, scan duration: %d sec", scanDuration);
  soapSSDPquery(&rcvd, scanDuration * 1000);

  log_i("SSDP query discovered %d media servers", rcvd.size());
  if (rcvd.size() == 0) return 0;   // return if none detected

  log_i("checking all discovered media servers for service ContentDirectory");

  int j = 0;
  uint64_t contentSize;
  bool chunked, gotFriendlyName, gotServiceType;
  String result((char *)0);
  MiniXPath xPath;
  soapServer_t srv;

  // examine all media servers that answered our SSDP multicast query
  while (j < rcvd.size()) {

    // try to establish connection to server and send GET request
    if (!soapGet(rcvd[j].ip, rcvd[j].port, rcvd[j].location.c_str())) goto end;
    log_i("connected successfully to server %s:%d", rcvd[j].ip.toString().c_str(), rcvd[j].port);

    // ok, connection established
    srv = { .ip = rcvd[j].ip, .port = rcvd[j].port, .location = rcvd[j].location, .friendlyName = "" };
    gotFriendlyName = false;
    gotServiceType = false;

    // reading HTTP header
    if (!soapReadHttpHeader(&contentSize, &chunked)) {
      goto end_stop_error;
    }
    if (!chunked && contentSize == 0) {  
      log_w("announced XML size: 0, we can stop here"); 
      goto end_stop_error;
    }  

    // scan XML block for description: friendly name, service type "ContentDirectory" & associated control URL
    xPath.reset();
    xPath.setPath(xmlParserPaths[xpFriendlyName].tagNames, xmlParserPaths[xpFriendlyName].num);
    while (true) {
      int ret = soapReadXML(chunked);
      if (ret < 0) {
        log_w("soapReadXML() returned: %d", ret); 
        goto end_stop_error;
      }       

      if (!gotFriendlyName) {
        if (xPath.getValue((char)ret, &result)) {
          srv.friendlyName = (result.length() > 0) ? result : "Server name not provided";
          log_d("scanned friendly name: %s", srv.friendlyName.c_str());
          gotFriendlyName = true;
          // we got friendly name and now set xPath for service type which comes next
          xPath.setPath(xmlParserPaths[xpServiceType].tagNames, xmlParserPaths[xpServiceType].num);
          continue;
        }  
      }
      else if (!gotServiceType) {
        if (xPath.getValue((char)ret, &result)) {
          if (strstr(result.c_str(),UPNP_URN_SCHEMA_CONTENT_DIRECTORY)) {
            log_d("server offers service: %s", UPNP_URN_SCHEMA_CONTENT_DIRECTORY);
            gotServiceType = true;
            // We got service type and now set xPath for control url (follows in same <service> block)
            xPath.setPath(xmlParserPaths[xpControlUrl].tagNames, xmlParserPaths[xpControlUrl].num);
            continue;
          }
        }
      }
      else if (xPath.getValue((char)ret, &result)) {
        // we finally got all infos we need
        if (srv.location.endsWith("/")) srv.controlURL = srv.location;  // location string becomes first part of controlURL 
        srv.controlURL += result;
        if (srv.controlURL.startsWith("http://")) {
          // remove "http://ip:port/" from begin of string
          srv.controlURL.replace("http://", "");        
          srv.controlURL = srv.controlURL.substring(srv.controlURL.indexOf("/") + 1); 
        }
        log_d("assigned controlURL: %s", srv.controlURL.c_str());
        log_i("ok, this server delivers media content");
        m_server.push_back(srv);  // add server to server list
        goto end_stop;
      }
    }
end_stop_error:
    log_i("this Server does not deliver media content");
end_stop:
    claimSPI();
    m_client->stop();
    releaseSPI();
end:
    j++;
  }

  return m_server.size();
}

//
// add a server manually to server list 
//
bool SoapESP32::addServer(IPAddress ip, uint16_t port, const char *controlURL, const char *name)
{
  soapServer_t srv;
  unsigned int i;
  
  // just some basic checks
  if (!ip || !port || 
      !name || strlen(name) == 0 ||
      !controlURL || strlen(controlURL) == 0) {
    log_e("at least on parameter is invalid");
    return false;
  }

  // refuse entry in list in case of identical ip & port
  for (i = 0; i < m_server.size(); i++) {
    if (m_server.operator[](i).ip == ip && m_server.operator[](i).port == port) break;               
  }
  if (i < m_server.size()) return false;

  srv.ip = ip;  
  srv.port = port;
  srv.controlURL = controlURL;
  srv.friendlyName = name;

  // add server to list
  m_server.push_back(srv);

  return true;
}

//
// erase all entries in server list
//
void SoapESP32::clearServerList()
{
  m_server.clear();
}

//
// helper function: scan for certain attribute
//
bool SoapESP32::soapScanAttribute(const String *attributes, String *result, const char *what)
{
  int begin, end;

  if ((begin = attributes->indexOf(what)) >= 0 &&
      (end = attributes->indexOf("\"", begin + strlen(what) + 1)) >= begin + strlen(what) + 1) {
    *result = attributes->substring(begin + strlen(what) + 1, end);
    if (result->length() > 0) return true;
  }

  *result = "";   // empty for next call
  log_i("attribute: \"%s\" missing.", what);

  return false;
}

//
// scan <container> content in SOAP answer
//
bool SoapESP32::soapScanContainer(const String *parentId, 
                                  const String *attributes, 
                                  const String *container, 
                                  soapObjectVect_t *browseResult)
{
  unsigned int i = 0;
  soapObject_t info;
  String str((char *)0);

  log_d("function entered, parent id: %s", parentId->c_str());

  // scan container id
  if (!soapScanAttribute(attributes, &str, DIDL_ATTR_ID)) return false;           // container id is a must
  log_d("%s\"%s\"", DIDL_ATTR_ID, str.c_str());
  info.id = str;

  // scan parent id
  if (!soapScanAttribute(attributes, &str, DIDL_ATTR_PARENT_ID)) return false;    // parent id is a must
  if (!strcasestr(str.c_str(), parentId->c_str())) {
#ifdef PARENT_ID_MUST_MATCH
    log_e("scanned parent id \"%s\" != requested parent id \"%s\"", str.c_str(), parentId->c_str());
    return false;
#else
    log_w("scanned parent id \"%s\" != requested parent id \"%s\"", str.c_str(), parentId->c_str());
#endif  
  }
  info.parentId = *parentId;  
  info.size = 0;
  info.sizeMissing = false;

  // scan child count...not always provided (e.g. Kodi)
  if (!soapScanAttribute(attributes, &str, DIDL_ATTR_CHILD_COUNT)) { 
    info.sizeMissing = true;
  }
  else {
    log_d("%s\"%s\"", DIDL_ATTR_CHILD_COUNT, str.c_str());
    if ((info.size = (int)str.toInt()) == 0) {
      log_i("container \"%s\" child count=0", info.id.c_str());
    }
  }

  // scan searchable flag...not always provided (e.g. UMS)
  if (!soapScanAttribute(attributes, &str, DIDL_ATTR_SEARCHABLE)) {
    log_i("attribute \"%s\" is missing, we set it true", DIDL_ATTR_SEARCHABLE);
    info.searchable = true;
  }  
  else {
    log_d("%s\"%s\"", DIDL_ATTR_SEARCHABLE, str.c_str());
    info.searchable = (1 == (int)str.toInt()) ? true : false;
    if (!info.searchable) {
      log_i("\"%s\" attribute searchable=0", info.id.c_str());
    }
  }

  // scan container name
  MiniXPath xPath;
  xPath.setPath(xmlParserPaths[xpTitle].tagNames, xmlParserPaths[xpTitle].num);
  while (i < container->length()) {
    if (xPath.getValue((char)container->operator[](i), &str)) {
      if (str.length() == 0) return false;    // valid title is a must
      info.name = str;
      log_d("title=\"%s\"", str.c_str());
      break;
    }
    i++;
  }

  // add valid container to result list
  info.isDirectory = true;
  browseResult->push_back(info);
  log_i("\"%s\" (id: \"%s\", childCount: %llu) added to list", info.name.c_str(), info.id.c_str(), info.size);

  return true; 
}

//
// scan <item> content in SOAP answer
//
bool SoapESP32::soapScanItem(const String *parentId, 
                             const String *attributes, 
                             const String *item, 
                             soapObjectVect_t *browseResult)
{
  soapObject_t info;
  unsigned int i = 0;
  int port;
  char address[20];
  IPAddress ip;
  String str((char *)0);

  log_d("function entered, parent id: %s", parentId->c_str());

  // scan item id
  if (!soapScanAttribute(attributes, &str, DIDL_ATTR_ID)) return false;         // id is a must
  log_d("%s\"%s\"", DIDL_ATTR_ID, str.c_str());
  info.id = str; 

  // scan parent id
  if (!soapScanAttribute(attributes, &str, DIDL_ATTR_PARENT_ID)) return false;  // parent id is a must
  if (!strcasestr(str.c_str(), parentId->c_str())) {
#ifdef PARENT_ID_MUST_MATCH
    log_e("scanned parent id \"%s\" != requested parent id \"%s\"", str.c_str(), parentId->c_str());
    return false;
#else
    log_w("scanned parent id \"%s\" != requested parent id \"%s\"", str.c_str(), parentId->c_str());
#endif
  }

  info.parentId = *parentId; 
  info.fileType = fileTypeOther;
  info.album = "";
  info.artist = "";
  info.genre = "";
  info.bitrate = 0;
  info.sampleFrequency = 0;
  info.size = 0;
  info.sizeMissing = false;

  // scan for uri, size, album (sometimes dir name when picture file) and title, artist (when audio file)
  MiniXPath xPathTitle, xPathAlbum, xPathArtist, xPathGenre, xPathClass, xPathRes;
  String strAttr((char *)0);
  bool gotTitle = false, gotAlbum = false, gotArtist = false, gotGenre = false, gotClass = false, gotRes = false;

  xPathTitle.setPath(xmlParserPaths[xpTitle].tagNames, xmlParserPaths[xpTitle].num);
  xPathAlbum.setPath(xmlParserPaths[xpAlbum].tagNames, xmlParserPaths[xpAlbum].num);
  xPathArtist.setPath(xmlParserPaths[xpArtist].tagNames, xmlParserPaths[xpArtist].num);
  xPathGenre.setPath(xmlParserPaths[xpGenre].tagNames, xmlParserPaths[xpGenre].num);
  xPathClass.setPath(xmlParserPaths[xpClass].tagNames, xmlParserPaths[xpClass].num);
  xPathRes.setPath(xmlParserPaths[xpResource].tagNames, xmlParserPaths[xpResource].num);
  while (i < item->length()) {
    if (!gotTitle && xPathTitle.getValue((char)item->operator[](i), &str)) {
      if (str.length() == 0) return false;    // valid title is a must 
      info.name = str;
      log_d("%s=\"%s\"", xmlParserPaths[xpTitle].tagNames[0], str.c_str());
      gotTitle = true;
    }
    if (!gotAlbum && xPathAlbum.getValue((char)item->operator[](i), &str)) {
      info.album = str;                       // missing album not a showstopper
      log_d("%s=\"%s\"", xmlParserPaths[xpAlbum].tagNames[0], str.c_str());;
      gotAlbum = true;
    }
    if (!gotArtist && xPathArtist.getValue((char)item->operator[](i), &str)) {
      info.artist = str;                      // missing artist not a showstopper
      log_d("%s=\"%s\"", xmlParserPaths[xpArtist].tagNames[0], str.c_str());
      gotArtist = true;
    }
    if (!gotGenre && xPathGenre.getValue((char)item->operator[](i), &str)) {
      info.genre = str;                       // missing genre not a showstopper
      log_d("%s=\"%s\"", xmlParserPaths[xpGenre].tagNames[0], str.c_str());
      gotGenre = true;
    }
    if (!gotClass && xPathClass.getValue((char)item->operator[](i), &str)) {
      log_d("%s=\"%s\"", xmlParserPaths[xpClass].tagNames[0], str.c_str());
      if (str.indexOf("audioItem") >= 0) 
        info.fileType = fileTypeAudio;
      else if (str.indexOf("imageItem") >= 0) 
        info.fileType = fileTypeImage;
      else if (str.indexOf("videoItem")) 
        info.fileType = fileTypeVideo;
      else 
        info.fileType = fileTypeOther;
      gotClass = true;
    }
    if (!gotRes && xPathRes.getValue((char)item->operator[](i), &str, &strAttr)) {
      if (str.startsWith("http://")) {
        // scan for download ip & port
        if (sscanf(str.c_str(), "http://%[0-9.]:%d/", address, &port) != 2) return false;
        info.downloadPort = (uint16_t)port;
        if (!ip.fromString(address)) return false;
        info.downloadIp = ip;
        // remove "http://ip:port/" from begin of string
        str.replace("http://", "");        
        info.uri = str.substring(str.indexOf("/") + 1); 
      }
      else {
        info.uri = str;
      }
      if (info.uri.length() == 0) return false;   // valid URI is a must     
      log_d("uri=\"%s\"", info.uri.c_str());

      // scan item size
      if (!soapScanAttribute(&strAttr, &str, DIDL_ATTR_SIZE)) {
        // indicates missing attribute "size" (e.g. Kodi audio files, Fritzbox/Serviio stream items)
        info.sizeMissing = true;
      } 
      else {
        info.size = strtoull(str.c_str(), NULL, 10);
        log_d("size=%llu", info.size);
      }
#if !defined(SHOW_EMPTY_FILES)
      if (info.size == 0 && !info.sizeMissing) {        
        log_w("reported size=0, item ignored"); 
        return false;
      }
#endif        

      // scan bitrate (often provided when audio file)
      if (soapScanAttribute(&strAttr, &str, DIDL_ATTR_BITRATE)) {
        info.bitrate = (size_t)str.toInt();
        if (info.bitrate == 0) {
          log_w("bitrate=0 !"); 
        }              
        else { 
          log_d("bitrate=%d", info.bitrate);
        }  
      }  

      // scan sample frequency (often provided when audio file)
      if (soapScanAttribute(&strAttr, &str, DIDL_ATTR_SAMPLEFREQU)) {
        info.sampleFrequency = (size_t)str.toInt();
        if (info.sampleFrequency == 0) {
          log_w("sampleFrequency=0");   
        }            
        else { 
          log_d("sampleFrequency=%d", info.sampleFrequency);
        }  
      }  

      gotRes = true;
    }
    i++;
  }

  if (!gotTitle || !gotRes) {
    log_i("title or ressource info missing, file not added to list");
    return false;   // title & ressource info is a must
  }  

  // add valid file to result list
  info.isDirectory = false;
  browseResult->push_back(info);

  log_i("\"%s\" (id: \"%s\", size: %llu, sizeMissing: %s, type: %s) added to list", 
        info.name.c_str(), info.id.c_str(), info.size, info.sizeMissing ? "true" : "false", getFileTypeName(info.fileType));

  return true;
}

//
// Process browse and search requests
//
bool SoapESP32::soapProcessRequest(const unsigned int srv,         // server number in list
                                   const char *objectId,           // directory to search, "0" represents root according to spec
                                   soapObjectVect_t *result,       // where to store browse/search results
                                   const char *searchCriteria,     // what to search for, e.g. "upnp:artist contains \"Name\""
                                   const char *sortCriteria,       // sort criteria for results returned
                                   const uint32_t startingIndex,   // offset into content list
                                   const uint16_t maxCount)        // limits number of objects in result list
{
  if (srv >= m_server.size()) {
    log_e("invalid server number: %d", srv);
    return false;
  }

  bool search = (searchCriteria != NULL);
  if (search)
    log_i("search server: \"%s\", objectId: \"%s\", searchCriteria: %s, sortCriteria: %s", 
           m_server[srv].friendlyName.c_str(), objectId, searchCriteria, sortCriteria);
  else 
    log_i("browse server: \"%s\", objectId: \"%s\"", m_server[srv].friendlyName.c_str(), objectId);

  if (startingIndex != (search ? SOAP_DEFAULT_SEARCH_STARTING_INDEX : SOAP_DEFAULT_BROWSE_STARTING_INDEX)) 
    log_d("special parameter for \"startingIndex\": %d", startingIndex);
  if (maxCount != (search ? SOAP_DEFAULT_SEARCH_MAX_COUNT : SOAP_DEFAULT_BROWSE_MAX_COUNT)) 
    log_d("special parameter for \"maxCount\": %d", maxCount);

  // send SOAP browse/search request to server
  if (!soapPost(m_server[srv].ip, m_server[srv].port, m_server[srv].controlURL.c_str(), objectId,
                searchCriteria, sortCriteria, startingIndex, maxCount)) {
    return false;
  }  
  log_i("connected successfully to server %s:%d", m_server[srv].ip.toString().c_str(), m_server[srv].port);

  // evaluate SOAP answer
  uint64_t contentSize;
  bool chunked = false;
  int count = 0, countContainer = 0, countItem = 0;
  MiniXPath xPathContainer, xPathContainerAlt1, xPathContainerAlt2,
            xPathItem, xPathItemAlt1, xPathItemAlt2,
            xPathNumberReturned, xPathNumberReturnedAlt1, xPathNumberReturnedAlt2;
  String str((char *)0), strAttribute((char *)0);

  // reading HTTP header
  if (!soapReadHttpHeader(&contentSize, &chunked)) {
    log_e("HTTP Header not ok or reply status not 200");
    claimSPI();
    m_client->stop();
    releaseSPI();
    return false;
  }
  if (!chunked && contentSize == 0) {  
    log_e("announced XML size: 0 !"); 
    claimSPI();
    m_client->stop();
    releaseSPI();    
    return false;
  } 
  log_i("scan answer from media server:"); 

  // time to clean result list
  result->clear();

  // HTTP header ok, now scan XML/SOAP reply
  String objId = objectId;  
  int eNum = search ? xpSearchContainer : xpBrowseContainer;
  xPathContainer.setPath(xmlParserPaths[eNum].tagNames, xmlParserPaths[eNum++].num);
  xPathContainerAlt1.setPath(xmlParserPaths[eNum].tagNames, xmlParserPaths[eNum++].num);
  xPathContainerAlt2.setPath(xmlParserPaths[eNum].tagNames, xmlParserPaths[eNum++].num);
  xPathItem.setPath(xmlParserPaths[eNum].tagNames, xmlParserPaths[eNum++].num);
  xPathItemAlt1.setPath(xmlParserPaths[eNum].tagNames, xmlParserPaths[eNum++].num);
  xPathItemAlt2.setPath(xmlParserPaths[eNum].tagNames, xmlParserPaths[eNum++].num);
  xPathNumberReturned.setPath(xmlParserPaths[eNum].tagNames, xmlParserPaths[eNum++].num);
  xPathNumberReturnedAlt1.setPath(xmlParserPaths[eNum].tagNames, xmlParserPaths[eNum++].num);
  xPathNumberReturnedAlt2.setPath(xmlParserPaths[eNum].tagNames, xmlParserPaths[eNum].num);
  while (true) {
    int ret = soapReadXML(chunked, true);  // de-chunk data stream and replace XML-entities (if found)
    if (ret < 0) {
      log_e("soapReadXML() returned: %d%s", ret, ret == -1 ? " (likely EOF)" : ""); 
      goto end_stop;
    }  
    // TEST
    //Serial.print((char)ret);
    //
    // only one will return a result
    if (xPathContainer.getValue((char)ret, &str, &strAttribute, true) ||
        xPathContainerAlt1.getValue((char)ret, &str, &strAttribute, true) ||
        xPathContainerAlt2.getValue((char)ret, &str, &strAttribute, true)) {
      log_v("container attribute (length=%d): %s", strAttribute.length(), strAttribute.c_str());
      log_v("container (length=%d): %s", str.length(), str.c_str());
      if (soapScanContainer(&objId, &strAttribute, &str, result))
        countContainer++;
      // TEST
      delay(1); // resets task switcher watchdog
    }
    if (xPathItem.getValue((char)ret, &str, &strAttribute, true) ||
        xPathItemAlt1.getValue((char)ret, &str, &strAttribute, true) ||
        xPathItemAlt2.getValue((char)ret, &str, &strAttribute, true)) {
      log_v("item attribute (length=%d): %s", strAttribute.length(), strAttribute.c_str());
      log_v("item (length=%d): %s", str.length(), str.c_str());
      if (soapScanItem(&objId, &strAttribute, &str, result))
        countItem++;
      // TEST
      delay(1); // resets task switcher watchdog
    }
    if (xPathNumberReturned.getValue((char)ret, &str) ||
        xPathNumberReturnedAlt1.getValue((char)ret, &str) ||
        xPathNumberReturnedAlt2.getValue((char)ret, &str)) {
      count = str.toInt();
      log_d("announced number of folders and/or files: %d", count);
      break;  // numberReturned comes last, so we can break here
    }
  }

  if (count == 0) {
    log_i("XML scanned, no elements announced");
  }
  else if (count != (countContainer + countItem)) {
    log_w("XML scanned, elements announced %d != found %d (possible reason: empty file or vital attributes missing)", count, countContainer + countItem);
  }

end_stop:
  claimSPI();
  m_client->stop();
  releaseSPI();
  log_i("found %d folders and %d files", countContainer, countItem);

  // TEST
#if CORE_DEBUG_LEVEL > 3
  delay(10);	// allow pending serial monitor output to be sent, can be massive when verbose
#elif CORE_DEBUG_LEVEL > 0  
  delay(2);
#endif  

  return true;
}

//
// browse a SOAP container object (directory) on a media server for content 
//
bool SoapESP32::browseServer(const unsigned int srv,         // server number in list
                             const char *objectId,           // directory to browse, "0" represents root according to spec
                             soapObjectVect_t *browseResult, // where to store browse results (directory content)
                             // optional parameter
                             const uint32_t startingIndex,   // offset into directory content list
                             const uint16_t maxCount)        // limits number of objects in result list
{
  return soapProcessRequest(srv, objectId, browseResult, NULL, NULL, startingIndex, maxCount);
}

//
// send a search request for files matching a special criteria to media server
//
bool SoapESP32::searchServer(const unsigned int srv,         // server number in list
                             const char *objectId,           // start directory to search from, "0" for root
                             soapObjectVect_t *searchResult, // where to store search results (file list)
                             const char *searchCriteria1,    // search criteria, e.g. "dc:title contains"
                             const char *param1,             // first criteria's parameter, e.g. "word"
                             // optional parameter
                             const char *searchCriteria2,    // optional search criteria, e.g. "upnp:class derivedfrom"
                             const char *param2,             // 2nd criteria's parameter, e.g. "object.item.videoItem"
                             const char *sortCriteria,       // optional sort criteria for results returned
                             const uint32_t startingIndex,   // offset into content list
                             const uint16_t maxCount)        // limits number of objects in result list
{
  String search((char *)0), sort((char *)0);

  if (searchCriteria1 == NULL) return false;

  // assemble final search criteria string
  if (param1 != NULL) 
    search = String(searchCriteria1) + " \"" + param1 + "\"";
  if (searchCriteria2 != NULL) 
    search += String(" and ") + searchCriteria2;
  if (param2 != NULL) 
    search += String(" \"") + param2 + "\"";

  // define sort criteria string  
  sort = (sortCriteria == NULL) ? SOAP_DEFAULT_SEARCH_SORT_CRITERIA : sortCriteria;

  return soapProcessRequest(srv, objectId, searchResult, search.c_str(), sort.c_str(), startingIndex, maxCount);
}

//
// Querying a media server's search/sort capabilities
//
bool SoapESP32::getServerCapabilities(const unsigned int srv, eCapabilityType capability, soapServerCapVect_t *result)
{
  if (srv >= m_server.size()) {
    log_e("invalid server number: %d", srv);
    return false;
  }

  log_i("querying %s capabilities from server: \"%s\"", capability == capSearch ? "search" : "sort", m_server[srv].friendlyName.c_str());

  // send SOAP browse/search request to server
  if (!soapPostCapabilities(m_server[srv].ip, m_server[srv].port, m_server[srv].controlURL.c_str(), capability)) {
    return false;
  }  
  log_i("connected successfully to server %s:%d", m_server[srv].ip.toString().c_str(), m_server[srv].port);

  // evaluate SOAP answer
  uint64_t contentSize;
  bool chunked = false;
  MiniXPath xPathCaps, xPathCapsAlt1, xPathCapsAlt2;
  String strCaps((char *)0);

  // reading HTTP header
  if (!soapReadHttpHeader(&contentSize, &chunked)) {
    log_e("HTTP Header not ok or reply status not 200");
    claimSPI();
    m_client->stop();
    releaseSPI();
    return false;
  }
  if (!chunked && contentSize == 0) {  
    log_e("announced XML size: 0 !"); 
    claimSPI();
    m_client->stop();
    releaseSPI();    
    return false;
  } 
  log_i("scan answer from media server:"); 

  result->clear();

  // HTTP header ok, now scan XML/SOAP reply
  int eNum = (capability == capSearch) ? xpGetSearchCapabilities : xpGetSortCapabilities;
  xPathCaps.setPath(xmlParserPaths[eNum].tagNames, xmlParserPaths[eNum++].num);
  xPathCapsAlt1.setPath(xmlParserPaths[eNum].tagNames, xmlParserPaths[eNum++].num);
  xPathCapsAlt2.setPath(xmlParserPaths[eNum].tagNames, xmlParserPaths[eNum].num);

  while (true) {
    int ret = soapReadXML(chunked, true);  // de-chunk data stream and replace XML-entities (if found)
    if (ret < 0) {
      log_e("soapReadXML() returned: %d%s", ret, ret == -1 ? " (likely EOF)" : ""); 
      goto end_stop;
    }  
    // TEST
    //Serial.print((char)ret);
    //
    if (xPathCaps.getValue((char)ret, &strCaps) ||
        xPathCapsAlt1.getValue((char)ret, &strCaps) ||
        xPathCapsAlt2.getValue((char)ret, &strCaps)) {
      log_v("\n%sCaps (length=%d): \"%s\"", capability == capSearch ? "Search" : "Sort", strCaps.length(), strCaps.c_str());
      break;
    }
  }

if (strCaps.length()) {
  unsigned int start = 0, index;
  String strItem((char *)0);

  // itemize the comma separated list of capabilities
  do {
    if ((index = strCaps.indexOf(',', start)) >= 0) {
      strItem = strCaps.substring(start, index);
    }
    else {
      strItem = strCaps.substring(start);
    }
    start += strItem.length() + 1;
    if (strItem.length()) result->push_back(strItem);
  }
  while (start < strCaps.length());
}

end_stop:
  claimSPI();
  m_client->stop();
  releaseSPI();

  // TEST
#if CORE_DEBUG_LEVEL > 3
  delay(5);	// allow pending serial monitor output to be sent
#elif CORE_DEBUG_LEVEL > 0  
  delay(2);
#endif  

  return true;
}

//
// request object (file) from media server
//
bool SoapESP32::readStart(soapObject_t *object, size_t *size)
{
  uint64_t contentSize;
  bool chunked;

  if (object->isDirectory) return false;

  log_i("server ip: %s, port: %d, uri: \"%s\"", 
        object->downloadIp.toString().c_str(), object->downloadPort, object->uri.c_str());

  // just to make sure old connection is closed
  if (m_clientDataConOpen) {
    claimSPI();
    m_client->stop();
    releaseSPI();
    m_clientDataConOpen = false;
    log_w("client data connection to media server was still open. Closed now.");
  }

  // establish connection to server and send GET request
  if (!soapGet(object->downloadIp, object->downloadPort, object->uri.c_str())) {
    return false;
  }

  // connection established, read HTTP header
  if (!soapReadHttpHeader(&contentSize, &chunked)) {
    // error returned
    log_e("soapReadHttpHeader() was unsuccessful.");
    claimSPI();
    m_client->stop();
    releaseSPI();
    return false;
  }

  // max allowed file size for download is 4.2GB (SIZE_MAX)
  if (contentSize > (uint64_t)SIZE_MAX) {
    log_e("file too big for download. Maximum allowed file size is 4.2GB.");
    claimSPI();
    m_client->stop();
    releaseSPI();
    return false;
  }
  
  m_clientDataAvailable = 0;
  m_clientDataChunked = chunked;
  m_ChunkCount = 0;

  if (contentSize > 0) {
    // file size announced in HTTP header
    log_d("media file size taken from http header: %llu", contentSize);
    m_clientDataAvailable = (size_t)contentSize;
  }
  else if (object->size > 0) {
    // as an alternative we use file size given in function argument
    log_d("media file size taken from argument (media object): %llu", object->size);
    m_clientDataAvailable = (size_t)object->size;
  }

  if (m_clientDataAvailable == 0) {  
    // no file size given or no data available to read
    log_e("unknown file size !"); 
    claimSPI();
    m_client->stop();
    releaseSPI();
    return false;
  } 

  m_clientDataConOpen = true;
  if (size) {                            // pointer valid ?
    *size = m_clientDataAvailable;       // return size of file
  }

  return true; 
}

//
// read up to size bytes from server and place them into buf
// returnes number of bytes read, -1 in case of read timeout or -2...-5 in case of other errors
// Remarks: 
// - older WiFi library versions & the Ethernet library return -1 if connection is still up but 
//   momentarily no data available and return 0 in case of EOF. Newer WiFi versions return 0 in 
//   both cases, so we need to treat -1 & 0 equally.
// - timeout checking is vital because client.read() can return 0 for ages in case of WiFi problems
//
int SoapESP32::read(uint8_t *buf, size_t size, uint32_t timeout) {

  // first some basic checks
  if (!buf || !size || !m_clientDataConOpen) return -1;  // clearly an error
  if (!m_clientDataAvailable) return 0;                  // most probably EOF

  int res = -1;  
  uint32_t start = millis();
  
  while (1) {
    //if (m_clientDataAvailable < size) size = m_clientDataAvailable;
    if (!m_clientDataChunked) {
      claimSPI();
      res = m_client->read(buf, size);
      releaseSPI();
    }
    else {
      // de-chunking of data required   
      if (m_ChunkCount <= 0) {
        char tmpBuffer[10];
      
        // next line contains chunk size
        claimSPI();
        int len = m_client->readBytesUntil('\n', tmpBuffer, sizeof(tmpBuffer) - 1);
        releaseSPI();
        if (len < 2) {
          log_e("error reading chunk size");     
          return -2;   // we expect at least 1 digit chunk size + '\r'
        }
        tmpBuffer[len-1] = 0;           // clear '\r'
        if (sscanf(tmpBuffer, "%x", &m_ChunkCount) != 1) {
          log_e("error scanning chunk size");  
          return -3;
        }
        log_d("announced chunk size: 0x%x(%d)", m_ChunkCount, m_ChunkCount);
        if (m_ChunkCount <= 0) {
          return -4;                    // not necessarily an error...final chunk size can be 0
        }
      }
      // read maximal till end of chunk
      if (m_ChunkCount < size) size = m_ChunkCount;
      claimSPI();
      res = m_client->read(buf, size);
      releaseSPI();
      if (res > 0) {
        m_ChunkCount -= res;
        // check for end of chunk
        if (m_ChunkCount == 0) {
          // skip "\r\n" trailing each chunk
          if (soapClientTimedRead(10) < 0 || soapClientTimedRead(10) < 0) {
            log_e("error reading chunk trailing CR+LF");  
            return -5;   
          }
        }
      }
    }
    if (res > 0) {
      // got at least 1 byte from server
      m_clientDataAvailable -= res;
      break;
    }  
    if ((millis() - start) > timeout) {
      // read timeout
      log_e("error, read timeout: %d ms", timeout);
      break;
    }
  }

  return res;
}

//
// read a single byte from server, return -1 in case of error
//
int SoapESP32::read(void)
{
  uint8_t b;
  if (read(&b, 1) > 0) return b;
  return -1;
}

//
// final stuff to be done after last read() call
//
void SoapESP32::readStop()
{
  if (m_clientDataConOpen) {
    claimSPI();
    m_client->stop();
    releaseSPI();
    m_clientDataConOpen = false;
    log_d("client data connection to media server closed");
  }
  m_clientDataAvailable = 0;
  m_clientDataChunked = false;
  m_ChunkCount = 0;
}

//
// HTTP GET request
//
bool SoapESP32::soapGet(const IPAddress ip, const uint16_t port, const char *uri)
{
  if (m_clientDataConOpen) {  
    // should not happen...probably buggy main
    claimSPI();
    m_client->stop();
    releaseSPI();
    m_clientDataConOpen = false;
    log_w("client data connection to media server was still open. Closed now.");
  }

  for (int i = 0;;) {
    claimSPI();
    bool ret = m_client->connect(ip, port);
    releaseSPI();
    if (ret) break;
    if (++i >= 3) {
      log_e("error connecting to server ip=%s, port=%d", ip.toString().c_str(), port);
      return false;
    }
    delay(100);  
  }

  // memory allocation for assembling HTTP header
  size_t length = strlen(uri) + 25;
  char *buffer = (char *)malloc(length);
  if (!buffer) {
    log_e("malloc() couldn't allocate memory");    
    return false;
  }
  String str((char *)0);

  // assemble HTTP header
  snprintf(buffer, length, "GET /%s %s", uri, HTTP_VERSION);
  str += buffer;
  log_d("%s:%d %s", ip.toString().c_str(), port, buffer);
  str += "\r\n";
  snprintf(buffer, length, HEADER_HOST, ip.toString().c_str(), port);
  str += buffer;
  str += HEADER_CONNECTION_CLOSE;
  str += HEADER_USER_AGENT;
  str += HEADER_EMPTY_LINE;           // empty line marks end of HTTP header

  // send request to server
  claimSPI();
  m_client->print(str);
  releaseSPI();

  // give server some time to answer
  uint32_t start = millis();
  while (true) {
    claimSPI();
    int av = m_client->available();
    releaseSPI();
    if (av) break;
    if (millis() > (start + SERVER_RESPONSE_TIMEOUT)) {
      claimSPI();
      m_client->stop();
      releaseSPI();
      log_e("GET: no reply from server for %d ms", SERVER_RESPONSE_TIMEOUT);
      free(buffer);
      return false;
    }
  }
  free(buffer);

  return true;
}

//
// HTTP POST request (browse/search)
//
bool SoapESP32::soapPost(const IPAddress ip, 
                         const uint16_t port, 
                         const char *uri, 
                         const char *objectId, 
                         const char *searchCriteria,                               
                         const char *sortCriteria,                               
                         const uint32_t startingIndex, 
                         const uint16_t maxCount)
{
  if (m_clientDataConOpen) {  
    // should not get here...probably buggy main
    claimSPI();
    m_client->stop();
    releaseSPI();
    m_clientDataConOpen = false;
    log_w("client data connection to media server was still open. Closed now.");
  }

  for (int i = 0;;) {
    claimSPI();
    int ret = m_client->connect(ip, (uint16_t)port);
    releaseSPI();
    if (ret) break;
    if (++i >= 2) {
      log_e("error connecting to server ip=%s, port=%d", ip.toString().c_str(), port);
      return false;
    }  
    delay(100);  
  }

  // memory allocation for assembling HTTP header
  size_t length = strlen(uri) + 30;
  char *buffer = (char *)malloc(length);
  if (!buffer) {
    log_e("malloc() couldn't allocate memory");    
    return false;
  }

  bool search = (searchCriteria != NULL);
  uint16_t messageLength;
  char index[12], count[6];
  String str((char *)0), 
         searchSortCriteria = (sortCriteria != NULL) ? sortCriteria : SOAP_DEFAULT_SEARCH_SORT_CRITERIA; 

  itoa(startingIndex, index, 10);
  itoa(maxCount, count, 10);
  // calculate XML message length
  messageLength = sizeof(SOAP_ENVELOPE_START) - 1;
  messageLength += sizeof(SOAP_BODY_START) - 1;
  messageLength += search ? (sizeof(SOAP_SEARCH_START) - 1) : (sizeof(SOAP_BROWSE_START) - 1);
  messageLength += search ? (sizeof(SOAP_CONTAINERID_START) - 1 + strlen(objectId) + sizeof(SOAP_CONTAINERID_END) - 1) :
                            (sizeof(SOAP_OBJECTID_START) - 1 + strlen(objectId) + sizeof(SOAP_OBJECTID_END) - 1);
  messageLength += search ? (sizeof(SOAP_SEARCHCRITERIA_START) - 1 + strlen(searchCriteria) + sizeof(SOAP_SEARCHCRITERIA_END) - 1) :
                            (sizeof(SOAP_BROWSEFLAG_START) - 1 + sizeof(SOAP_DEFAULT_BROWSE_FLAG) - 1 + sizeof(SOAP_BROWSEFLAG_END) - 1);
  messageLength += search ? (sizeof(SOAP_FILTER_START) - 1 + sizeof(SOAP_DEFAULT_SEARCH_FILTER) - 1 + sizeof(SOAP_FILTER_END) - 1) :
                            (sizeof(SOAP_FILTER_START) - 1 + sizeof(SOAP_DEFAULT_BROWSE_FILTER) - 1 + sizeof(SOAP_FILTER_END) - 1);
  messageLength += sizeof(SOAP_STARTINGINDEX_START) - 1 + strlen(index) + sizeof(SOAP_STARTINGINDEX_END) - 1;
  messageLength += sizeof(SOAP_REQUESTEDCOUNT_START) - 1 + strlen(count) + sizeof(SOAP_REQUESTEDCOUNT_END) - 1;
  messageLength += search ? (sizeof(SOAP_SORTCRITERIA_START) - 1 + strlen(searchSortCriteria.c_str()) + sizeof(SOAP_SORTCRITERIA_END) - 1) :
                            (sizeof(SOAP_SORTCRITERIA_START) - 1 + sizeof(SOAP_DEFAULT_BROWSE_SORT_CRITERIA) - 1 + sizeof(SOAP_SORTCRITERIA_END) - 1);
  messageLength += search ? (sizeof(SOAP_SEARCH_END) - 1) : (sizeof(SOAP_BROWSE_END) - 1);
  messageLength += sizeof(SOAP_BODY_END) - 1;
  messageLength += sizeof(SOAP_ENVELOPE_END) - 1;

  // assemble HTTP header
  snprintf(buffer, length, "POST /%s %s", uri, HTTP_VERSION);
  str += buffer;
  log_d("%s:%d %s", ip.toString().c_str(), port, buffer);
  str += "\r\n";
  snprintf(buffer, length, HEADER_HOST, ip.toString().c_str(), port); // 29 bytes max
  str += buffer;
  // TEST
  str += "CACHE-CONTROL: no-cache\r\nPRAGMA: no-cache\r\n";
  //str += "FRIENDLYNAME.DLNA.ORG: ESP32-Radio\r\n";
  //
  str += HEADER_CONNECTION_CLOSE;
  snprintf(buffer, length, HEADER_CONTENT_LENGTH_D, messageLength);
  str += buffer;
  str += HEADER_CONTENT_TYPE;
  str += search ? HEADER_SOAP_ACTION_SEARCH : HEADER_SOAP_ACTION_BROWSE;
  str += HEADER_USER_AGENT;
  str += HEADER_EMPTY_LINE;                    // empty line marks end of HTTP header

  // assemble SOAP message (multiple str+= instead of a single str+=..+..+.. reduces allocation depth)
  str += SOAP_ENVELOPE_START;
  str += SOAP_BODY_START;
  str += search ? SOAP_SEARCH_START : SOAP_BROWSE_START;
  str += search ? SOAP_CONTAINERID_START : SOAP_OBJECTID_START;
  str += objectId;
  str += search ? SOAP_CONTAINERID_END : SOAP_OBJECTID_END;
  str += search ? SOAP_SEARCHCRITERIA_START : SOAP_BROWSEFLAG_START;
  str += search ? searchCriteria : SOAP_DEFAULT_BROWSE_FLAG;
  str += search ? SOAP_SEARCHCRITERIA_END : SOAP_BROWSEFLAG_END;
  str += SOAP_FILTER_START;
  str += search ? SOAP_DEFAULT_SEARCH_FILTER : SOAP_DEFAULT_BROWSE_FILTER;
  str += SOAP_FILTER_END;
  str += SOAP_STARTINGINDEX_START;
  str += index;
  str += SOAP_STARTINGINDEX_END;
  str += SOAP_REQUESTEDCOUNT_START;
  str += count;
  str += SOAP_REQUESTEDCOUNT_END;
  str += SOAP_SORTCRITERIA_START;
  str += search ? searchSortCriteria.c_str() : SOAP_DEFAULT_BROWSE_SORT_CRITERIA;
  str += SOAP_SORTCRITERIA_END;
  str += search ? SOAP_SEARCH_END : SOAP_BROWSE_END;
  str += SOAP_BODY_END;
  str += SOAP_ENVELOPE_END;

  // send request to server
  log_v("send %s request to server:\n%s", search ? "search" : "browse", str.c_str());
  claimSPI();
  m_client->print(str);
  releaseSPI();

  // wait for a reply until timeout
  uint32_t start = millis();
  while (true) {
    claimSPI();
    int av = m_client->available();
    releaseSPI();
    if (av) break;
    if (millis() > (start + SERVER_RESPONSE_TIMEOUT)) {
      claimSPI();
      m_client->stop();
      releaseSPI();
      log_e("POST: no reply from server within %d ms", SERVER_RESPONSE_TIMEOUT);
      free(buffer);
      return false;
    }
  }
  free(buffer);

  return true;
}

//
// HTTP POST request (search/sort capabilities)
//
bool SoapESP32::soapPostCapabilities(const IPAddress ip, 
                                     const uint16_t port, 
                                     const char *uri, 
                                     eCapabilityType capability)
{
  if (m_clientDataConOpen) {  
    // should not get here...probably buggy main
    claimSPI();
    m_client->stop();
    releaseSPI();
    m_clientDataConOpen = false;
    log_w("client data connection to media server was still open. Closed now.");
  }

  for (int i = 0;;) {
    claimSPI();
    int ret = m_client->connect(ip, (uint16_t)port);
    releaseSPI();
    if (ret) break;
    if (++i >= 2) {
      log_e("error connecting to server ip=%s, port=%d", ip.toString().c_str(), port);
      return false;
    }  
    delay(100);  
  }

  // memory allocation for assembling HTTP header
  size_t length = strlen(uri) + 30;
  char *buffer = (char *)malloc(length);
  if (!buffer) {
    log_e("malloc() couldn't allocate memory");    
    return false;
  }

  uint16_t messageLength;
  String str((char *)0);

  // calculate XML message length
  messageLength = sizeof(SOAP_ENVELOPE_START) - 1;
  messageLength += sizeof(SOAP_BODY_START) - 1;
  messageLength += capability == capSort ? (sizeof(SOAP_GETSORTCAP_START) - 1) : (sizeof(SOAP_GETSEARCHCAP_START) - 1);
  messageLength += capability == capSort ? (sizeof(SOAP_GETSORTCAP_END) - 1) : (sizeof(SOAP_GETSEARCHCAP_END) - 1);
  messageLength += sizeof(SOAP_BODY_END) - 1;
  messageLength += sizeof(SOAP_ENVELOPE_END) - 1;

  // assemble HTTP header
  snprintf(buffer, length, "POST /%s %s", uri, HTTP_VERSION);
  str += buffer;
  log_d("%s:%d %s", ip.toString().c_str(), port, buffer);
  str += "\r\n";
  snprintf(buffer, length, HEADER_HOST, ip.toString().c_str(), port); // 29 bytes max
  str += buffer;
  // TEST
  str += "CACHE-CONTROL: no-cache\r\nPRAGMA: no-cache\r\n";
  //str += "FRIENDLYNAME.DLNA.ORG: ESP32-Radio\r\n";
  //
  str += HEADER_CONNECTION_CLOSE;
  snprintf(buffer, length, HEADER_CONTENT_LENGTH_D, messageLength);
  str += buffer;
  str += HEADER_CONTENT_TYPE;
  str += capability == capSort ? HEADER_SOAP_ACTION_GETSORTCAP : HEADER_SOAP_ACTION_GETSEARCHCAP;
  str += HEADER_USER_AGENT;
  str += HEADER_EMPTY_LINE;                    // empty line marks end of HTTP header

  // assemble SOAP message (multiple str+= instead of a single str+=..+..+.. reduces allocation depth)
  str += SOAP_ENVELOPE_START;
  str += SOAP_BODY_START;
  str += capability == capSort ? SOAP_GETSORTCAP_START : SOAP_GETSEARCHCAP_START;
  str += capability == capSort ? SOAP_GETSORTCAP_END : SOAP_GETSEARCHCAP_END;
  str += SOAP_BODY_END;
  str += SOAP_ENVELOPE_END;

  // send request to server
  log_v("send %s request to server:\n%s", capability == capSort ? "GetSortCapabilities" : "GetSearchCapabilities", str.c_str());
  claimSPI();
  m_client->print(str);
  releaseSPI();

  // wait for a reply until timeout
  uint32_t start = millis();
  while (true) {
    claimSPI();
    int av = m_client->available();
    releaseSPI();
    if (av) break;
    if (millis() > (start + SERVER_RESPONSE_TIMEOUT)) {
      claimSPI();
      m_client->stop();
      releaseSPI();
      log_e("POST: no reply from server within %d ms", SERVER_RESPONSE_TIMEOUT);
      free(buffer);
      return false;
    }
  }
  free(buffer);

  return true;
}

//
// returns number of usable media servers in our list
//
unsigned int SoapESP32::getServerCount(void)
{
  return m_server.size();
}

//
// returns infos about a media servers from our internal list
//
bool SoapESP32::getServerInfo(unsigned int srv, soapServer_t *serverInfo)
{
  if (srv >= m_server.size()) return false;
  *serverInfo = m_server[srv];

  return true;
}

//
// returns number of available/remaining bytes
//
size_t SoapESP32::available()
{
  return m_clientDataConOpen ? m_clientDataAvailable : 0;
}

//
// returns pointer to string (item type name)
//
const char *SoapESP32::getFileTypeName(eFileType fileType)
{
  return (fileTypeAudio <= fileType && fileType <= fileTypeVideo) ? fileTypes[fileType] : fileTypes[fileTypeOther];
}
