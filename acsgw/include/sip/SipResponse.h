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

#include "SipParsedMessage.h"

#include <map>
#include <string>
#include <memory>

/* -------------------------------------------------------------------------- */

class SipResponse
{
public:
    using Handle = std::shared_ptr<SipResponse>;

    SipResponse() = delete;
    SipResponse(const SipResponse &) = default;
    SipResponse &operator=(const SipResponse &) = default;

    SipResponse(const SipParsedMessage &request, int code);

    const std::string &getRespBuffer() const noexcept
    {
        return _response;
    }

    bool isPositive() const noexcept {
        return _responseCode>=200 && _responseCode<300;
    }

private:
    int _responseCode = 0;
    std::string _response;

    static std::unordered_map<int, const char*> _SipResponseCodeTable;
};
