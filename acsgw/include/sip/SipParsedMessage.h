//
// This file is part of acsgw
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.
// Licensed under the MIT License.
// See COPYING file in the project root for full license information.
//

/* -------------------------------------------------------------------------- */

#pragma once

/* -------------------------------------------------------------------------- */

#include "SipHeaderParser.h"
#include "SipHeader.h"

#include "TextTokenizer.h"

#include <iostream>
#include <list>
#include <memory>
#include <string>
#include <cstdio>
#include <map>
#include <string.h>
#include <string>

/* -------------------------------------------------------------------------- */

class SipParsedMessage
{
public:
   using Handle = std::shared_ptr<SipParsedMessage>;
   using ParsedHeaderMap = std::map<std::string, SipHeader::Handle>;
   using HeaderList = std::list<std::string>;

   SipParsedMessage() = default;

   SipParsedMessage(const SipParsedMessage &other) : _sipParser(other._sipParser),
                                                     _headerList(other._headerList),
                                                     _responseCode(other._responseCode)
   {
      for (auto const &[key, val] : _parsedHeaderMap)
      {
         _parsedHeaderMap[key] = val->clone();
      }
   }

   SipParsedMessage &operator=(const SipParsedMessage &other)
   {
      if (this != &other)
      {
         _sipParser = other._sipParser;
         _headerList = other._headerList;
         _responseCode = other._responseCode;

         for (auto const &[key, val] : _parsedHeaderMap)
         {
            _parsedHeaderMap[key] = val->clone();
         }
      }
      return *this;
   }

   const HeaderList &getHeaderList() const noexcept
   {
      return _headerList;
   }

   void parseHeader(const std::string &header);

   const ParsedHeaderMap &getParsedHeaderMap() const
   {
      return _parsedHeaderMap;
   }

   int getResponseCode() const noexcept
   {
      return _responseCode;
   }

   bool isResposeOK() const noexcept {
      return _responseCode >=200 && _responseCode < 300;
   }

   const std::string& getMethodName() const noexcept 
   {
      return _messageMethodName;
   }

   std::ostream &dump(std::ostream &os, const std::string &id = "");

private:
   SipHeaderParser _sipParser;
   HeaderList _headerList;
   ParsedHeaderMap _parsedHeaderMap;
   int _responseCode = 0;
   std::string _messageMethodName;
};
