/*
  MiniXPath is part of SoapESP32 library and used for scanning XML streams.
  It is based on Thomas Mittets (code@lookout.no) "MicroXPath" but needs
  substantially more RAM and the following features have been added:
  - extracting attributes
  - extracting whole sub trees if requested
  - using C++ strings
  - path not required to start at root element
*/

#include "MiniXPath.h"

MiniXPath::MiniXPath()
{
  pathSize = 0;
  reset();
}

void MiniXPath::reset()
{
  state = XML_PARSER_UNINITIATED;
  sub = 0;
  position = 0;
  treeFlag = false;
  matchCount = 0;       
  tagLevel = 0;
  subLevel = 0;
  matchLevel = 0;
#ifdef MINIXPATH_DEBUG	
  Serial.printf("reset: tagLev=%d sub=%d subLev=%d paSize=%d paMatched=%d stat=%02d matchCnt=%d path=%s\n", 
                  tagLevel, sub, subLevel, pathSize, matchLevel, state, matchCount, pathSize == 0 ? "" : path[pathSize-1]);
  Serial.flush();
  delay(2);
#endif									
}

void MiniXPath::setPath(const xPathParser_t *p)
{
  uint8_t newMatchLevel = 0;
  for (uint8_t i = 0; i < p->num && i < pathSize && i < matchLevel && i == newMatchLevel; i++) {
    if (strcmp(p->tagNames[i], this->path[i]) == 0) newMatchLevel++;
  }
  matchCount = 0;
  matchLevel = newMatchLevel;
  path = (char **)p->tagNames;
  pathSize = p->num;
  if (subLevel == 0) sub = p->sub;
#ifdef MINIXPATH_DEBUG	
  Serial.printf("setPath: tagLev=%d sub=%d subLev=%d paSize=%d paMatched=%d stat=%02d matchCnt=%d pos=%d path=%s\n", 
                  tagLevel, sub, subLevel, pathSize, matchLevel, state, matchCount, position, pathSize == 0 ? "" : path[pathSize - 1]);
  Serial.flush();
  delay(2);
#endif									
}

bool MiniXPath::findValue(char charToParse)
{
  return find(charToParse, false) && state == XML_PARSER_ELEMENT_CONTENT;
}

bool MiniXPath::getValue(char charToParse, String *result, String *attrib, bool subTree) 
{
  if (find(charToParse, subTree)) {
    if (subTree) {
      if (treeFlag) {
        *result += charToParse;    
        if (state == XML_PARSER_END_TAG && tagLevel - subLevel == matchLevel) {
          if (result->endsWith("</")) result->remove(result->length() - 2);
          result->trim();
          return true;
        }    
      }
    }
    // ignoring sub elements
    else if (pathSize == matchLevel) {
      if (state == XML_PARSER_ELEMENT_CONTENT &&        
          position > 0) { // skips the tag-start/end characters and trailing whitespace
        *result += charToParse;
      }
      else if (state == XML_PARSER_END_TAG && position == 0) {
        result->trim();
        return true;
      }
    }
    // getting attribute data
    if (attrib != NULL && (state == XML_PARSER_ATTRIBUTES || state == XML_PARSER_ATTRIBUTE_VALUE) && 
        position > 0 && matchLevel > 0 && (tagLevel - subLevel == matchLevel - 1)) {
      if (charToParse == '\t' || charToParse == '\r' || charToParse == '\n') {
        *attrib += ' ';
      }  
      else {
        *attrib += charToParse;
      }  
    }
  }
  else if (tagLevel - subLevel == matchLevel && state == XML_PARSER_START_TAG && position == 0) {
    // making sure we start clean
    if (attrib != NULL) *attrib = "";
    *result = "";
#ifdef MINIXPATH_DEBUG	
    Serial.println("getValue: clear attrib & result");
#endif
  }
  return false;
}

bool MiniXPath::find(char charToParse, bool subTree)
{
  // parsing starts with first "<" character
  if (state == XML_PARSER_UNINITIATED && charToParse == '<') state = XML_PARSER_ROOT;
  if (state >= XML_PARSER_COMPLETE && charToParse > ' ') {}
  else if (state > XML_PARSER_UNINITIATED && state < XML_PARSER_COMPLETE) {
    switch (charToParse) {
      // Tag start
      case '<':
        if (matchLevel < pathSize) treeFlag = false;
        else if (subTree) treeFlag = true;
        if (state == XML_PARSER_ROOT || state == XML_PARSER_ELEMENT_CONTENT) {
          state = XML_PARSER_START_TAG;
        }
        position = 0;
        matchCount = 0;
        break;
      // Tag end
      case '>':
        if (matchLevel < pathSize) treeFlag = false;
        if (state == XML_PARSER_START_TAG_NAME || state == XML_PARSER_ATTRIBUTES) {
          if (elementPathMatch()) {
            matchLevel++;
            if (sub) {
              sub = false;
              subLevel = tagLevel; 
            }
          }
          tagLevel++;
          state = XML_PARSER_ELEMENT_CONTENT;
        }
        else if (state == XML_PARSER_END_TAG) {
          if (tagLevel - subLevel == matchLevel && matchLevel > 0) 
            matchLevel--;
          tagLevel--;
          if (tagLevel > 0) {
            state = XML_PARSER_ELEMENT_CONTENT;
          }
          else {
            //state = XML_PARSER_COMPLETE;
            state = XML_PARSER_ROOT;
          }  
        }
        else if (tagLevel == 0 && state == XML_PARSER_PROLOG_END) {
          state = XML_PARSER_ROOT;
        }
        else if (state == XML_PARSER_COMMENT) {
          state = (tagLevel == 0) ? XML_PARSER_ROOT : XML_PARSER_ELEMENT_CONTENT;
        }
        position = 0;
        matchCount = 0;
        break;
      // Prolog start and end character
      case '?':
        if (matchLevel < pathSize) treeFlag = false;
        if (state == XML_PARSER_START_TAG && tagLevel == 0) {
          state = XML_PARSER_PROLOG_TAG;
        }
        else if (state == XML_PARSER_PROLOG_TAG_NAME || state == XML_PARSER_PROLOG_ATTRIBUTES) {
          state = XML_PARSER_PROLOG_END;
        }
        break;
      // Comment start character
      case '!':
        //if (tagLevel < pathSize) treeFlag = false;
        if (state == XML_PARSER_START_TAG) {
          state = XML_PARSER_COMMENT;
        }
        break;
      // Attribute start character and end character        
      case '"':
      case '\'':
        if (tagLevel < pathSize) treeFlag = false;
        position++;
        switch (state)
        {
          case XML_PARSER_ATTRIBUTES:
            state = XML_PARSER_ATTRIBUTE_VALUE;
            position++;
            break;
          case XML_PARSER_ATTRIBUTE_VALUE:
            state = XML_PARSER_ATTRIBUTES;
            position++;
            break;
        }
        break;
      // End tag character
      case '/':
        if (state == XML_PARSER_START_TAG) {
          state = XML_PARSER_END_TAG;
        }
        else if (state == XML_PARSER_START_TAG_NAME) {
          if (elementPathMatch()) {
            matchLevel++;
            position = 0;
            if (sub) {
              sub = false;
              subLevel = tagLevel; 
            }
          }            
          tagLevel++;
          state = XML_PARSER_END_TAG;
        }
        else if (state == XML_PARSER_ATTRIBUTES) {
          tagLevel++;
          state = XML_PARSER_END_TAG;          
        }
        else if (state == XML_PARSER_ELEMENT_CONTENT)
          position++;
        break;
      // Whitespace
      case ' ':
      case '\t':
      case '\r':
      case '\n':
        if (tagLevel - subLevel < pathSize) treeFlag = false;
        switch (state) {
          case XML_PARSER_START_TAG_NAME:
            if (elementPathMatch()) {
              matchLevel++;
              if (sub) {
                sub = false;
                subLevel = tagLevel; 
              }
            }
            state = XML_PARSER_ATTRIBUTES;
            position = 0;
            break;
          case XML_PARSER_PROLOG_TAG_NAME:
            state = XML_PARSER_PROLOG_ATTRIBUTES;
            break;
          case XML_PARSER_ATTRIBUTES:
          case XML_PARSER_ATTRIBUTE_VALUE:
            position++;
            break;
        }
        break;        
      // All other characters
      default:
        if (tagLevel - subLevel < pathSize ||
            (subTree && state == XML_PARSER_END_TAG && tagLevel - subLevel == matchLevel)) {
          treeFlag = false;
        }
        else if (subTree && matchLevel == pathSize) treeFlag = true;  
        if (state == XML_PARSER_START_TAG) state = XML_PARSER_START_TAG_NAME;
        else if (state == XML_PARSER_PROLOG_TAG) state = XML_PARSER_PROLOG_TAG_NAME;
        if (state == XML_PARSER_START_TAG_NAME && matchCount == position && 
            (sub ? true : (tagLevel - subLevel < pathSize)) &&
            position < strlen(path[matchLevel]) && charToParse == path[matchLevel][position]) {
          matchCount++;
        }
        position++;
        break;
    }
  }
#ifdef MINIXPATH_DEBUG	
  // TEST
  //if (pathSize == 1 && (*(path[0]) == 'c' || *(path[0]) == 'i')) {
  char charTP = (charToParse < 0x20) ? '.' : charToParse;
  Serial.printf("'%c' tagLev=%d sub=%d subLev=%d paSize=%d paMatched=%d stat=%02d matchCnt=%d pos=%d treeFlg=%d ret=%d\n", 
                  charTP, tagLevel, (int)sub, (int)subLevel, pathSize, matchLevel, state, matchCount, position, treeFlag, matchLevel == pathSize);  
  //}                
#endif									
  return (matchLevel == pathSize);
}

bool MiniXPath::elementPathMatch()
{
  return (matchLevel < pathSize) && (matchCount == position) && 
         (sub ? true : (tagLevel - subLevel < pathSize)) &&
         (matchCount == strlen(path[matchLevel]));
}
