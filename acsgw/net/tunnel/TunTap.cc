//
// This file is part of acsgw
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.  
// Licensed under the MIT License. 
// See COPYING file in the project root for full license information.
//

/* -------------------------------------------------------------------------- */

#include "TunTap.h"
#include "IpAddress.h"

#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <memory.h>

/* -------------------------------------------------------------------------- */

static bool assign_ip(const std::string &ifname, const std::string ip)
{
    struct ifreq ifr;
    struct sockaddr_in sin;
    int sd = socket(AF_INET, SOCK_DGRAM, 0);

    memset(&sin, 0, sizeof(sin));
    strncpy(ifr.ifr_name, ifname.c_str(), IFNAMSIZ);
    sin.sin_family = AF_INET;

    if (!ip.empty())
    {
        sin.sin_addr.s_addr = inet_addr(ip.c_str());

        memcpy(&ifr.ifr_addr, &sin, sizeof(struct sockaddr));

        if (ioctl(sd, SIOCSIFADDR, &ifr) < 0)
        {
            return false;
        }
    }

    if (ioctl(sd, SIOCGIFFLAGS, (void *)&ifr) < 0)
    {
        return false;
    }

    if (ifr.ifr_flags | ~(IFF_UP))
    {
        ifr.ifr_flags |= IFF_UP;
        ioctl(sd, SIOCSIFFLAGS, (void *)&ifr);
    }

    close(sd);

    return true;
}

/* -------------------------------------------------------------------------- */

TunTap ::TunTap(const std::string &ifname, const std::string &ip, int flags)
{
    _fd = open("/dev/net/tun", O_RDWR);

    if (_fd < 0)
    {
        throw Exception::OPEN_ERROR;
    }
    else
    {
        struct ifreq ifr;

        memset(&ifr, 0, sizeof(ifr));
        ifr.ifr_flags = flags;
        strncpy(ifr.ifr_name, ifname.c_str(), IFNAMSIZ);

        if (ioctl(_fd, TUNSETIFF, (void *)&ifr) < 0)
        {
            close(_fd);
            _fd = -1;
            throw Exception::IOCTL_ERROR;
        }

        if (!assign_ip(ifname, ip))
        {
            close(_fd);
            _fd = -1;
            throw Exception::IOCTL_ERROR;
        }
    }
}
