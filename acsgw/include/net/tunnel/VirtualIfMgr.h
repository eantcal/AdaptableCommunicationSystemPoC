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

#include "MacAddress.h"
#include "IpAddress.h"
#include "TunTap.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string>
#include <unistd.h>

#include <memory>
#include <unordered_map>

/* -------------------------------------------------------------------------- */

class VirtualIfMgr
{
public:
   enum
   {
      MAX_PKT_SIZE = 64 * 1024
   };

private:
   std::unordered_map<std::string, std::shared_ptr<TunTap>> _devs;

public:
   VirtualIfMgr() = default;

   ssize_t addIf(
       const std::string &ifname
       //int mtu
       ) noexcept;

   ssize_t announcePacket(
       const std::string &ifname,
       const char *data,
       size_t datalen) noexcept;

   ssize_t getPacket(
       char *buf,
       size_t bufsize,
       std::string &ifname) noexcept;
};
