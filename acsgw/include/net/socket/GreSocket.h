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

#include "IpAddress.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cstdint>

/* -------------------------------------------------------------------------- */

class GreSocket
{
public:
   enum class PollingState
   {
      RECEIVING_DATA,
      TIMEOUT_EXPIRED,
      ERROR_IN_COMMUNICATION
   };

   typedef uint16_t PortType;

private:
   int _sock = -1;

   static void _format_sock_addr(
       sockaddr_in &sa,
       const IpAddress &addr) noexcept;

   //copy not allowed
   GreSocket(const GreSocket &) = delete;
   GreSocket &operator=(const GreSocket &) = delete;

public:
   GreSocket(GreSocket &&s) noexcept
   {
      //xfer socket descriptor to constructing object
      _sock = s._sock;
      s._sock = -1;
   }

   GreSocket &operator=(GreSocket &&s) noexcept
   {
      if (&s != this)
      {
         //xfer socket descriptor to constructing object
         _sock = s._sock;
         s._sock = -1;
      }

      return *this;
   }

   int getSocketDesc() const noexcept
   {
      return _sock;
   }

   bool isValid() const noexcept
   {
      return getSocketDesc() > -1;
   }

   GreSocket() noexcept;

   virtual ~GreSocket() noexcept;

   PollingState poll(struct timeval &timeout) const noexcept;

   bool close() noexcept;

   int sendto(
       const char *buf,
       int len,
       const IpAddress &ip,
       int flags = 0) const noexcept;

   int recvfrom(
       char *buf,
       int len,
       IpAddress &src_addr,
       int &payloadOffset,
       int flags = 0) const noexcept;

   bool bind(
       const IpAddress &ip = IpAddress(INADDR_ANY),
       bool reuse_addr = true) const noexcept;
};
