//
// This file is part of acsgw
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.  
// Licensed under the MIT License. 
// See COPYING file in the project root for full license information.
//

/* -------------------------------------------------------------------------- */

#include "GreSocket.h"

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
#include <assert.h>
#include <unistd.h>
#include <linux/if_packet.h>

/* -------------------------------------------------------------------------- */

void GreSocket::_format_sock_addr(sockaddr_in &sa,
                                  const IpAddress &addr) noexcept
{
   memset(&sa, 0, sizeof(sa));

   sa.sin_family = AF_INET;
   sa.sin_addr.s_addr = htonl(addr.to_uint32());
   sa.sin_port = htons(IPPROTO_GRE);
}

/* -------------------------------------------------------------------------- */

GreSocket::GreSocket() noexcept : _sock(::socket(AF_INET, SOCK_RAW, IPPROTO_GRE))
{
   //assert(_sock > -1); // TODO
}

/* -------------------------------------------------------------------------- */

GreSocket::~GreSocket() noexcept
{
   close();
}

/* -------------------------------------------------------------------------- */

int GreSocket::sendto(
    const char *buf,
    int len,
    const IpAddress &ip,
    int flags) const noexcept
{
   struct sockaddr_in remote_host = {0};

   _format_sock_addr(remote_host, ip);

   int bytes_sent = ::sendto(getSocketDesc(),
                             buf, len,
                             flags,
                             (struct sockaddr *)&remote_host,
                             sizeof(remote_host));

   return bytes_sent;
}

/* -------------------------------------------------------------------------- */

/*
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |Version|  IHL  |Type of Service|          Total Length         |  0-3
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |         Identification        |Flags|      Fragment Offset    |  4-7
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Time to Live |    Protocol   |         Header Checksum       |  8-11
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                       Source Address                          |  12-15
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                    Destination Address                        |  16-19
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                    Options                    |    Padding    |  20-23
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

int GreSocket::recvfrom(
    char *buf,
    int len,
    IpAddress &src_addr,
    int &payloadOffset,
    int flags) const noexcept

{
   enum
   {
      IHL_MIN_BLEN = 20,
      IHL_MAX_BLEN = 60,
      GRE_PROTO_OFFSET = 2,
      GRE_PROTO_IP = 0x0800,
      IP_SRC_ADDR_OFFSET = 12,
      GRE_HEADER_LEN = 4
   };

   int n = recv(getSocketDesc(), buf, len, 0);

   if (n < 0)
      return -1;

   // Compute IP Header length (4 * low nibble of first byte of IP packet)
   int ihl = (buf[0] & 0x0f) << 2;

   // Validate IHL
   if (ihl > IHL_MAX_BLEN || ihl < IHL_MIN_BLEN)
      return -1;

   // Parse GRE header (expected all 0s)
   if (*(uint16_t *)(buf + ihl) != 0)
      return 0;

   // Expected Protocol code 0x800
   uint16_t protocol = ntohs(*(uint16_t *)(buf + ihl + GRE_PROTO_OFFSET));
   if (protocol != GRE_PROTO_IP)
      return 0;

   // Source IP address (of physical interface) is at offset 12 of IP packet
   src_addr = IpAddress(htonl(*((uint32_t *)&buf[IP_SRC_ADDR_OFFSET])));

   // IP packet payload starts after IP Header + 4 bytes for GRE Header
   payloadOffset = ihl + GRE_HEADER_LEN;

   return n - ihl - GRE_HEADER_LEN;
}

/* -------------------------------------------------------------------------- */

GreSocket::PollingState GreSocket::poll(struct timeval &timeout) const noexcept
{
   fd_set readMask;
   FD_ZERO(&readMask);
   FD_SET(getSocketDesc(), &readMask);

   long nd = 0;

   do
   {
      nd = ::select(FD_SETSIZE, &readMask, 0, 0, &timeout);
   } while (!FD_ISSET(_sock, &readMask) && nd > 0);

   if (nd <= 0)
      return nd == 0 ? PollingState::TIMEOUT_EXPIRED : PollingState::ERROR_IN_COMMUNICATION;

   return PollingState::RECEIVING_DATA;
}

/* -------------------------------------------------------------------------- */

bool GreSocket::bind(
    const IpAddress &ip,
    bool reuse_addr) const noexcept
{
   struct sockaddr_in sin = {0};
   socklen_t address_len = 0;

   if (reuse_addr)
   {
      int bool_val = 1;

      setsockopt(
          getSocketDesc(),
          SOL_SOCKET,
          SO_REUSEADDR,
          (const char *)&bool_val,
          sizeof(bool_val));
   }

   _format_sock_addr(sin, ip);

   int sock = getSocketDesc();
   int err_code = ::bind(sock, (struct sockaddr *)&sin, sizeof(sin));

   return err_code == 0;
}

/* -------------------------------------------------------------------------- */

bool GreSocket::close() noexcept
{
   int sock = _sock;
   _sock = -1;

   return ::close(sock) == 0;
}

/* -------------------------------------------------------------------------- */
