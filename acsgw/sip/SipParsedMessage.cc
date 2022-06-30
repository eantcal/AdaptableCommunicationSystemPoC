//
// This file is part of acsgw
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.
// Licensed under the MIT License.
// See COPYING file in the project root for full license information.
//

/* -------------------------------------------------------------------------- */

#include "SipParsedMessage.h"
#include "Logger.h"

#include <sstream>

/* -------------------------------------------------------------------------- */

std::ostream &SipParsedMessage::dump(std::ostream &os, const std::string &id)
{
   std::string ss;

   if (!id.empty())
      ss = "MSG(" + id + ")\n";

   for (auto e : getHeaderList())
      ss += e;

   os << ss << "\n";

   return os;
}

/* -------------------------------------------------------------------------- */

void SipParsedMessage::parseHeader(const std::string &header)
{
   try
   {
      SipTnkzr inlinetknzr(header, 0);
      SipHeader::Handle h(_sipParser.parse(inlinetknzr));

      if (h) {
         const std::string& headerName = h->name();
         auto res = _parsedHeaderMap.insert({headerName, h});

         if (h->getType() == SipHeader::HeaderType::RESPONSE)
         {
            _responseCode = h->getResponseCode();
         }
         else if (h->getType() == SipHeader::HeaderType::METHOD)
         {
            _messageMethodName = h->name();
         }

         if (res.second == true) 
         {
            _headerList.push_back(header);
         }
         else 
         {
            SipHeaderParser sipParser;
            for (auto& existingHeader: _headerList) {
               SipTnkzr tknzr(existingHeader, 0);
               SipHeader::Handle hField(sipParser.parse(tknzr));
               if (hField && hField->name()==headerName) {
                  existingHeader = header;
                  break;
               }
            }
         }

         std::stringstream ss;
         h->describe(ss);
         TRACE(LOG_DEBUG, "SipParsedMessage::parseHeader %s", ss.str().c_str());
      }
      else 
      {
         TRACE(LOG_ERR, "SipParsedMessage::parseHeader failed on parsing header");
      }
   }
   catch (Exception &e)
   {
      TRACE(LOG_ERR, "SipParsedMessage::parseHeader exception:%s", e.what());
   }
}
