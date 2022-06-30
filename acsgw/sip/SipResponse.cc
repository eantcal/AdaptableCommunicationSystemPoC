//
// This file is part of acsgw
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.  
// Licensed under the MIT License. 
// See COPYING file in the project root for full license information.
//


/* -------------------------------------------------------------------------- */

#include "SipResponse.h"
#include "Tools.h"
#include "GlobalDefs.h"

#include <sstream>
#include <unordered_map>

/* -------------------------------------------------------------------------- */

std::unordered_map<int, const char*> SipResponse::_SipResponseCodeTable 
{
    {100, "100 Trying"},
    {200, "200 OK"},
    {400, "400 Bad Request"},
    {401, "401 Unauthorized"},
    {403, "403 Forbidden"},
    {404, "404 Not Found"},
    {405, "405 Method Not Allowed"},
    {408, "408 Request Timeout"},
    {480, "480 Temporarily Unavailable"}
};


/* -------------------------------------------------------------------------- */

SipResponse::SipResponse(
    const SipParsedMessage& request, int responseCode) : 
    _responseCode(responseCode)
{
    std::stringstream ss;

    std::string desc = 
        responseCode < 100 || responseCode >=700 ? "" : 
        _SipResponseCodeTable[responseCode];

    if (desc.empty())
        desc += "501 Not Implemented";

    // Example SIP/2.0 200 OK 
    _response = SIP_PROTO " " + desc + "\r\n"; 

    const auto& headers = request.getHeaderList();

    if (headers.size()>1) {
        auto it = headers.cbegin();
        ++it; // skip first line
        for (; it != headers.cend(); ++it) 
            _response += *it;

        _response += "\r\n"; // EOM
    }
}

/* -------------------------------------------------------------------------- */