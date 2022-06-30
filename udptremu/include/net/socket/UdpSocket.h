//
// This file is part of acsgw
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.  
// Licensed under the MIT License. 
// See COPYING file in the project root for full license information.
//


#pragma once


/* -------------------------------------------------------------------------- */

#include "TransportSocket.h"
#include "IpAddress.h"

#include <memory>
#include <string>
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

class UdpSocket 
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
         sockaddr_in & sa, 
         const IpAddress& addr, 
         PortType port) noexcept;
      
      //copy not allowed
      UdpSocket(const UdpSocket&) = delete;
      UdpSocket& operator=(const UdpSocket&) = delete;
     
   public:
      UdpSocket(UdpSocket&& s) noexcept
      {
         //xfer socket descriptor to constructing object
         _sock = s._sock;
         s._sock = -1;
      }

      UdpSocket& operator=(UdpSocket&& s) noexcept
      {
         if (&s != this)
         {  
            //xfer socket descriptor to constructing object
            _sock = s._sock;
            s._sock = -1;
         }

         return *this;
      }


      int getSd() const noexcept 
      { 
         return _sock; 
      }


      bool isValid() const noexcept
      { 
         return getSd() > -1;
      }


      UdpSocket() noexcept;

      virtual ~UdpSocket() noexcept;

      PollingState poll(struct timeval& timeout) const noexcept;

      bool close() noexcept;

      int sendto(
          const char *buf,
          int len,
          const IpAddress &ip,
          PortType port,
          int flags = 0) const noexcept;

      int recvfrom(
          char *buf,
          int len,
          IpAddress &src_addr,
          PortType &src_port,
          int flags = 0) const noexcept;

      bool bind(
          PortType &port,
          const IpAddress &ip = IpAddress(INADDR_ANY),
          bool reuse_addr = true) const noexcept;
};

