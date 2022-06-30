//
// This file is part of acsgw
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.  
// Licensed under the MIT License. 
// See COPYING file in the project root for full license information.
//

/* -------------------------------------------------------------------------- */

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <features.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <assert.h>
#include <string>

#include "VirtualIfMgr.h"
#include "MacAddress.h"
#include "IpAddress.h"
#include "TunTap.h"

/* -------------------------------------------------------------------------- */

ssize_t VirtualIfMgr::addIf(const std::string &ifname) noexcept
{
   if (_devs.find(ifname) == _devs.end()) {
      try 
      {
         _devs.insert(std::make_pair(ifname, new TunTap(ifname)));
      }
      catch (TunTap::Exception e) 
      {
         return -1;
      }
   }
   else {
      return 0;
   }

   return 0;
}


/* -------------------------------------------------------------------------- */

ssize_t VirtualIfMgr::announcePacket(
    const std::string &ifname,
    const char *data,
    size_t datalen) noexcept
{
   auto it = _devs.find(ifname);

   if (it == _devs.end()) {
      return -1;
   }

   uint16_t dlen = datalen < MAX_PKT_SIZE ? datalen : MAX_PKT_SIZE;
   return it->second->writePacket(data, dlen);
}


/* -------------------------------------------------------------------------- */

ssize_t VirtualIfMgr::getPacket(char *buf, size_t bufsize,
                                std::string &ifname) noexcept
{
   assert(buf && bufsize >= 0);

   if (bufsize == 0)
      return 0;

   auto it = _devs.begin();

   if (it == _devs.end()) {
      return -1;
   }

   ifname = it->first;

   auto dev = _devs.begin()->second;

   assert(dev);

   return (ssize_t) dev->readPacket(buf, bufsize);
}

/* -------------------------------------------------------------------------- */
