//
// This file is part of acsgw
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.
// Licensed under the MIT License.
// See COPYING file in the project root for full license information.
//

/* -------------------------------------------------------------------------- */

#pragma once

#include <unistd.h>
#include <strings.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <string>

#include "IpAddress.h"

/* -------------------------------------------------------------------------- */

class TunTap
{
public:
    enum class Exception
    {
        OPEN_ERROR,
        IOCTL_ERROR
    };

    TunTap(const std::string &ifname,
           const std::string &ip = "",
           int flags = IFF_TUN | IFF_NO_PI);

    int readPacket(char *buf, size_t bufsize)
    {
        return read(_fd, buf, bufsize);
    }

    int writePacket(const char *buf, size_t wbutes)
    {
        return write(_fd, buf, wbutes);
    }

    ~TunTap()
    {
        close(_fd);
        _fd = -1;
    }

private:
    int _fd = -1;
};
