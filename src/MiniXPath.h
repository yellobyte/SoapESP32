/*
  MiniXPath is part of SoapESP32 library and used for scanning XML streams.
  It is based on Thomas Mittets (code@lookout.no) "MicroXPath" but needs
  substantially more RAM and the following features have been added:
  - extracting attributes
  - extracting whole sub trees if requested
  - using C++ strings
*/

#ifndef MiniXPath_h
#define MiniXPath_h

#include <Arduino.h>

#define XML_PARSER_UNINITIATED             0
#define XML_PARSER_ROOT                    1
#define XML_PARSER_START_TAG               2  // '<' 
#define XML_PARSER_START_TAG_NAME          3  // normal character detected (not >,<,/,CR,LF,etc...) in state 2
#define XML_PARSER_PROLOG_TAG              4  // "<?"
#define XML_PARSER_PROLOG_TAG_NAME         5
#define XML_PARSER_PROLOG_END              6  // '?' 
#define XML_PARSER_PROLOG_ATTRIBUTES       7
#define XML_PARSER_PROLOG_ATTRIBUTE_VALUE  8
#define XML_PARSER_ATTRIBUTES              9
#define XML_PARSER_ATTRIBUTE_VALUE        10
#define XML_PARSER_ELEMENT_CONTENT        11
#define XML_PARSER_COMMENT                12
#define XML_PARSER_END_TAG                13  // "</"
#define XML_PARSER_COMPLETE               14

#define XML_PROLOG "xml"

struct xPathParser_t
{ 
  const bool    sub;           // true if path starts not with root element
  const uint8_t num;           // number of path elements
  const char   *tagNames[10];  // can hold max 10 path elements
};

class MiniXPath {
  public:
    uint8_t state;
    
    MiniXPath();

    void reset();
    void setPath(const xPathParser_t *path);
    bool findValue(char charToParse);
    bool getValue(char charToParse, String *result, String *attrib = NULL, bool subTree = false);

  private:
    char     **path;
    uint8_t    pathSize;
    bool       sub;
    uint8_t    subLevel;       // level with first match if path doesn't start at root
    uint8_t    tagLevel;       // current tag level
    uint16_t   position;       // Position in string, set to 0 after scanning '<' or '>'
    bool       treeFlag;       // indicates data on matchlevel or above when whole sub tree is requested
    uint16_t   matchCount;     // tag chars already matched with path string, set to 0 after scanning '<' or '>'
    uint8_t    matchLevel;     // current match level after scanning path tags (pathsize is maximum)

    bool find(char charToParse, bool subTree);
    bool elementPathMatch();
};

#endif
