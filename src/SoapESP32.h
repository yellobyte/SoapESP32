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

#ifndef SoapESP32_h
#define SoapESP32_h

#include <Arduino.h>
#include <vector>

#if defined(ethernet_h_) && !defined(USE_ETHERNET)
#warning "== ATTENTION == Did you forget to define USE_ETHERNET ????  Please read Readme.md !!!!" 
#endif

#ifdef USE_ETHERNET
#include <Ethernet.h>
#else
#include <WiFi.h>
#endif

#define TMP_BUFFER_SIZE_200         200
#define TMP_BUFFER_SIZE_400         400
#define TMP_BUFFER_SIZE_1000       1000

// network communication timeouts
#define SERVER_RESPONSE_TIMEOUT    3000   // ms
#define SERVER_READ_TIMEOUT        3000   // ms

// SSDP UDP - seeking media servers
#define SSDP_MULTICAST_IP          239,255,255,250
#define SSDP_MULTICAST_PORT        1900
#define SSDP_SCAN_DURATION           60   // seconds, default network scan duration for seekServer()
#define SSDP_LOCATION_BUF_SIZE      150
#define SSDP_M_SEARCH_REPEATS         2
#define SSDP_M_SEARCH                "M-SEARCH * HTTP/1.1\r\nHOST: 239.255.255.250:1900\r\nMAN: \"ssdp:discover\"\r\nMX: 5\r\nST: "
#define SSDP_DEVICE_TYPE_MS          "urn:schemas-upnp-org:device:MediaServer:1"
#define SSDP_DEVICE_TYPE_RD          "upnp:rootdevice"
#define SSDP_SERVICE_TYPE_CD         "urn:schemas-upnp-org:service:ContentDirectory:1"                                   
#define SSDP_LOCATION                "Location: http://"
#define SSDP_NOTIFICATION            "NOTIFY * HTTP/1.1"
#define SSDP_NOTIFICATION_SUB_TYPE   "ssdp:alive"

// HTTP header lines
#define HTTP_VERSION                    "HTTP/1.1"
#define HTTP_HEADER_200_OK              "HTTP/1.1 200 OK"
#define HEADER_CONTENT_LENGTH           "Content-Length: "
#define HEADER_HOST                     "Host: %s:%d\r\n"
#define HEADER_CONTENT_TYPE             "Content-Type: text/xml; charset=\"utf-8\"\r\n"
#define HEADER_TRANS_ENC_CHUNKED        "Transfer-Encoding: chunked"
#define HEADER_CONTENT_LENGTH_D         "Content-Length: %d\r\n"
#define HEADER_SOAP_ACTION_BROWSE       "SOAPAction: \"urn:schemas-upnp-org:service:ContentDirectory:1#Browse\"\r\n"
#define HEADER_SOAP_ACTION_SEARCH       "SOAPAction: \"urn:schemas-upnp-org:service:ContentDirectory:1#Search\"\r\n"
#define HEADER_SOAP_ACTION_GETSEARCHCAP "SOAPAction: \"urn:schemas-upnp-org:service:ContentDirectory:1#GetSearchCapabilities\"\r\n"
#define HEADER_SOAP_ACTION_GETSORTCAP   "SOAPAction: \"urn:schemas-upnp-org:service:ContentDirectory:1#GetSortCapabilities\"\r\n"
#define HEADER_USER_AGENT               "User-Agent: ESP32/Player/UPNP1.0\r\n"
#define HEADER_CONNECTION_CLOSE         "Connection: close\r\n"
#define HEADER_CONNECTION_KEEP_ALIVE    "Connection: keep-alive\r\n"
#define HEADER_EMPTY_LINE               "\r\n"

// SOAP tag data
// TEST
//#define SOAP_ENVELOPE_START       "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\" " \
//                                  "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">\r\n"
//
#define SOAP_ENVELOPE_START        "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" " \
                                   "s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\r\n"
#define SOAP_ENVELOPE_END          "</s:Envelope>\r\n\r\n"
#define SOAP_BODY_START            "<s:Body>\r\n"
#define SOAP_BODY_END              "</s:Body>\r\n"
#define SOAP_BROWSE_START          "<u:Browse xmlns:u=\"urn:schemas-upnp-org:service:ContentDirectory:1\">\r\n"
#define SOAP_BROWSE_END            "</u:Browse>\r\n"
#define SOAP_SEARCH_START          "<u:Search xmlns:u=\"urn:schemas-upnp-org:service:ContentDirectory:1\">\r\n"
#define SOAP_SEARCH_END            "</u:Search>\r\n"
#define SOAP_GETSEARCHCAP_START    "<u:GetSearchCapabilities xmlns:u=\"urn:schemas-upnp-org:service:ContentDirectory:1\">\r\n"
#define SOAP_GETSEARCHCAP_END      "</u:GetSearchCapabilities>\r\n"
#define SOAP_GETSORTCAP_START      "<u:GetSortCapabilities xmlns:u=\"urn:schemas-upnp-org:service:ContentDirectory:1\">\r\n"
#define SOAP_GETSORTCAP_END        "</u:GetSortCapabilities>\r\n"
#define SOAP_OBJECTID_START        "<ObjectID>"
#define SOAP_OBJECTID_END          "</ObjectID>\r\n"
#define SOAP_CONTAINERID_START     "<ContainerID>"
#define SOAP_CONTAINERID_END       "</ContainerID>\r\n"
#define SOAP_BROWSEFLAG_START      "<BrowseFlag>"
#define SOAP_BROWSEFLAG_END        "</BrowseFlag>\r\n"
#define SOAP_FILTER_START          "<Filter>"
#define SOAP_FILTER_END            "</Filter>\r\n"
#define SOAP_STARTINGINDEX_START   "<StartingIndex>"
#define SOAP_STARTINGINDEX_END     "</StartingIndex>\r\n"
#define SOAP_REQUESTEDCOUNT_START  "<RequestedCount>"
#define SOAP_REQUESTEDCOUNT_END    "</RequestedCount>\r\n"
#define SOAP_SEARCHCRITERIA_START  "<SearchCriteria>"
#define SOAP_SEARCHCRITERIA_END    "</SearchCriteria>\r\n"
#define SOAP_SORTCRITERIA_START    "<SortCriteria>"
#define SOAP_SORTCRITERIA_END      "</SortCriteria>\r\n"

// UPnP/SOAP browse/search default parameters
#define UPNP_URN_SCHEMA_CONTENT_DIRECTORY SSDP_SERVICE_TYPE_CD
#define SOAP_DEFAULT_BROWSE_FLAG             "BrowseDirectChildren"
#define SOAP_DEFAULT_BROWSE_FILTER           "*"
#define SOAP_DEFAULT_BROWSE_SORT_CRITERIA    ""
#define SOAP_DEFAULT_BROWSE_STARTING_INDEX   0
#define SOAP_DEFAULT_BROWSE_MAX_COUNT        100     // arbitrary value to limit memory usage
#define SOAP_DEFAULT_SEARCH_FILTER           "*"
#define SOAP_DEFAULT_SEARCH_SORT_CRITERIA    ""
#define SOAP_DEFAULT_SEARCH_STARTING_INDEX   0
#define SOAP_DEFAULT_SEARCH_MAX_COUNT        100     // arbitrary value to limit memory usage

#define SOAP_SEARCH_CRITERIA_TITLE   "dc:title contains"
#define SOAP_SEARCH_CRITERIA_ARTIST  "upnp:artist contains"
#define SOAP_SEARCH_CRITERIA_ALBUM   "upnp:album contains" 
#define SOAP_SEARCH_CRITERIA_GENRE   "upnp:genre contains"  
#define SOAP_SEARCH_CRITERIA_CLASS   "upnp:class derivedfrom"
#define SOAP_SEARCH_CLASS_ALBUM      "object.container.album"
#define SOAP_SEARCH_CLASS_VIDEO      "object.item.videoItem" 
#define SOAP_SEARCH_CLASS_AUDIO      "object.item.audioItem" 
#define SOAP_SEARCH_CLASS_IMAGE      "object.item.imageItem" 
#define SOAP_SORT_TITLE_ASCENDING    "+dc:title"
#define SOAP_SORT_TITLE_DESCENDING   "-dc:title"
#define SOAP_SORT_ARTIST_ASCENDING   "+upnp:artist"
#define SOAP_SORT_ARTIST_DESCENDING  "-upnp:artist"
#define SOAP_SORT_ALBUM_ASCENDING    "+upnp:album"
#define SOAP_SORT_ALBUM_DESCENDING   "-upnp:album"

// selected DIDL attributes for scanning
#define DIDL_ATTR_ID           "id="
#define DIDL_ATTR_PARENT_ID    "parentID="
#define DIDL_ATTR_CHILD_COUNT  "childCount="
#define DIDL_ATTR_SEARCHABLE   "searchable="
#define DIDL_ATTR_SIZE         "size="
#define DIDL_ATTR_BITRATE      "bitrate="
#define DIDL_ATTR_SAMPLEFREQU  "sampleFrequency="

// for replacing predefined XML entities in server reply
enum eXmlReplaceState { xmlPassthrough = 0, xmlAmpDetected, xmlTakeFromBuffer };
struct replaceWith_t 
{
  const char *replace;
  const char with;
};

// defines the data content of a reported item (file/stream)
enum eFileType { fileTypeOther = 0, fileTypeAudio, fileTypeImage, fileTypeVideo };

// defines what capabilities to query from server
enum eCapabilityType { capSearch = 0, capSort };

typedef std::vector<String> soapServerCapVect_t;

// info collection of a single SOAP object (<container> or <item>) 
struct soapObject_t
{
  bool isDirectory;         // true if directory
  uint64_t size;            // directory child count or item size, zero in case of missing size/child count attribute
  bool sizeMissing;         // true in case server did not provide size
  int  bitrate;             // bitrate (music files only)
  int  sampleFrequency;     // sample frequency (music files only)
  bool searchable;          // only used for directories, some media servers don't provide it
  eFileType fileType;       // audio, picture, movie, stream or other
  String parentId;          // parent directory id
  String id;                // unique id of directory/file on media server
  String name;              // directory name or file name
  String artist;            // for music files
  String album;             // for music files (sometimes folder name when picture file)
  String genre;             // for audio/video files
  String uri;               // item URI on server, needed for download with readStart()
  IPAddress downloadIp;     // download IP can differ from server IP
  uint16_t downloadPort;    // download port can differ from server control port
};
typedef std::vector<soapObject_t> soapObjectVect_t;

// keeps vital infos of each media server
struct soapServer_t
{
  IPAddress ip;
  uint16_t port;
  String location;
  String friendlyName;
  String controlURL;
};
typedef std::vector<soapServer_t> soapServerVect_t;

// SoapESP32 class
class SoapESP32
{
  public:
#ifdef USE_ETHERNET
    SoapESP32(EthernetClient *client, EthernetUDP *udp = NULL, SemaphoreHandle_t *sem = NULL);
#else
    SoapESP32(WiFiClient *client, WiFiUDP *udp = NULL);
#endif
    bool          wakeUpServer(const char *macWOL);
    void          clearServerList(void);
    bool          addServer(IPAddress ip, uint16_t port, const char *controlURL, const char *name = "My Media Server");
    unsigned int  seekServer(unsigned int scanDuration = SSDP_SCAN_DURATION);
    unsigned int  getServerCount(void);
    bool          getServerInfo(unsigned int srv, soapServer_t *serverInfo);
    bool          getServerCapabilities(const unsigned int srv, eCapabilityType capability, soapServerCapVect_t *result);
    bool          browseServer(const unsigned int srv, const char *objectId, soapObjectVect_t *browseResult, 
                               const uint32_t startingIndex = SOAP_DEFAULT_BROWSE_STARTING_INDEX, 
                               const uint16_t maxCount      = SOAP_DEFAULT_BROWSE_MAX_COUNT);
    bool          searchServer(const unsigned int srv, const char *containerId, soapObjectVect_t *searchResult,
                               const char *searchCriteria1, const char *param1,
                               const char *searchCriteria2  = NULL,
                               const char *param2           = NULL,
                               const char *sortCriteria     = NULL,
                               const uint32_t startingIndex = SOAP_DEFAULT_SEARCH_STARTING_INDEX, 
                               const uint16_t maxCount      = SOAP_DEFAULT_SEARCH_MAX_COUNT);                             
    bool          readStart(soapObject_t *object, size_t *size);
    int           read(uint8_t *buf, size_t size, uint32_t timeout = SERVER_READ_TIMEOUT);
    int           read(void);
    void          readStop(void);
    size_t        available(void);
    const char*   getFileTypeName(eFileType fileType);

  private:
#ifdef USE_ETHERNET
    EthernetClient    *m_client;                // pointer to EthernetClient object
    EthernetUDP       *m_udp;                   // pointer to EthernetUDP object
    SemaphoreHandle_t *m_SPIsem;                // only needed if we use Ethernet lib instead of builtin WiFi
#else
    WiFiClient        *m_client;                // pointer to WiFiClient object
    WiFiUDP           *m_udp;                   // pointer to WiFiUDP object
#endif    
    bool               m_clientDataConOpen;     // marker: socket open for reading file
    size_t             m_clientDataAvailable;   // file read count
    soapServerVect_t   m_server ;               // list of usable media servers in local network
    int                m_xmlChunkCount;         // nr of bytes left of chunk (0 = end of chunk, next line delivers chunk size)
    eXmlReplaceState   m_xmlReplaceState;       // state machine for replacing XML entities
    uint8_t            m_xmlReplaceOffset;
    char               m_xmlReplaceBuffer[15];  // Fits longest string in replaceWith[] array

    int  soapClientTimedRead(void);
    bool soapUDPmulticast(unsigned int repeats = 0);
    bool soapSSDPquery(std::vector<soapServer_t> *rcvd, int msWait);
    bool soapGet(const IPAddress ip, const uint16_t port, const char *uri);
    bool soapPost(const IPAddress ip, const uint16_t port, const char *uri, const char *objectId, 
                  const char *searchCriteria, const char *sortCriteria, const uint32_t startingIndex, const uint16_t maxCount);                        
    bool soapPostCapabilities(const IPAddress ip, const uint16_t port, const char *uri, eCapabilityType capability);
    bool soapReadHttpHeader(uint64_t *contentLength, bool *chunked = NULL);
    int  soapReadXML(bool chunked = false, bool replace = false);
    bool soapScanAttribute(const String *attributes, String *result, const char *searchFor);
    bool soapScanContainer(const String *parentId, const String *attributes, const String *container, soapObjectVect_t *browseResult);
    bool soapScanItem(const String *parentId, const String *attributes, const String *item, soapObjectVect_t *browseResult);
    bool soapProcessRequest(const unsigned int srv, const char *objectId, soapObjectVect_t *result, const char *searchCriteria, 
                            const char *sortCriteria, const uint32_t startingIndex, const uint16_t maxCount); 
};

#endif