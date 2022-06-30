//
// This file is part of acsgw
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.
// Licensed under the MIT License.
// See COPYING file in the project root for full license information.
//

/* -------------------------------------------------------------------------- */

#include "UdpSocket.h"
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
#include <assert.h>


/* -------------------------------------------------------------------------- */

void UdpSocket::_format_sock_addr(sockaddr_in & sa,
      const IpAddress& addr,
      PortType port) noexcept
{
   memset(&sa, 0, sizeof(sa));

   sa.sin_family = AF_INET;
   sa.sin_addr.s_addr = htonl(addr.to_uint32());
   sa.sin_port = htons(port);
}


/* -------------------------------------------------------------------------- */

UdpSocket::UdpSocket() noexcept :
   _sock(::socket(AF_INET, SOCK_DGRAM, 0))
{
   assert( _sock > -1 );
}


/* -------------------------------------------------------------------------- */

UdpSocket::~UdpSocket() noexcept
{
   close();
}


/* -------------------------------------------------------------------------- */

int UdpSocket::sendto(
      const char* buf,
      int len,
      const IpAddress& ip,
      PortType port,
      int flags) const noexcept
{
   struct sockaddr_in remote_host = {0};

   _format_sock_addr(remote_host, ip, port);

   int bytes_sent = ::sendto(getSd(),
      buf, len,
      flags,
      (struct sockaddr*) &remote_host,
      sizeof(remote_host));

   return bytes_sent;
}


/* -------------------------------------------------------------------------- */

int UdpSocket::recvfrom(
      char *buf,
      int len,
      IpAddress& src_addr,
      PortType& src_port,
      int flags) const noexcept

{
   struct sockaddr_in source;
   socklen_t sourcelen = sizeof(source);
   memset(&source, 0, sizeof(source));

   int recv_result = ::recvfrom(
      getSd(), buf, len,
      flags, (struct sockaddr*)&source,
      &sourcelen);

   src_addr = IpAddress(htonl(source.sin_addr.s_addr));
   src_port = PortType(htons(source.sin_port));

   return recv_result;
}


/* -------------------------------------------------------------------------- */

UdpSocket::PollingState UdpSocket::poll(struct timeval& timeout) const noexcept
{
   fd_set readMask;
   FD_ZERO(&readMask);
   FD_SET(getSd(), &readMask);

   long nd = 0;

   do
   {
      nd = ::select (FD_SETSIZE, &readMask, 0, 0, &timeout);
   }
   while (! FD_ISSET(_sock, &readMask) && nd > 0);

   if (nd <= 0)
      return nd == 0 ?
         PollingState::TIMEOUT_EXPIRED :
         PollingState::ERROR_IN_COMMUNICATION;

   return PollingState::RECEIVING_DATA;
}


/* -------------------------------------------------------------------------- */

bool UdpSocket::bind(
   PortType& port,
   const IpAddress& ip,
   bool reuse_addr) const noexcept
{
   struct sockaddr_in sin = {0};
   socklen_t address_len = 0;

   if (reuse_addr)
   {
      int bool_val = 1;

      setsockopt(
         getSd(),
         SOL_SOCKET,
         SO_REUSEADDR,
         (const char*) &bool_val,
         sizeof(bool_val));
   }

   _format_sock_addr(sin, ip, port);

   int sock = getSd();
   int err_code = ::bind (sock, (struct sockaddr *) &sin, sizeof(sin));

   if (port == 0)
   {
      address_len = sizeof(sin);
      getsockname(sock, (struct sockaddr *) &sin, &address_len);
      port = ntohs(sin.sin_port);
   }

   // DEBUG only
   // printf("\n\n\nBIND %s:%i err=%i\n\n\n", ip.to_str().c_str(), int(port), err_code);
   // perror("bind");

   return err_code == 0;
}


/* -------------------------------------------------------------------------- */

bool UdpSocket::close() noexcept
{
   int sock = _sock;
   _sock = -1;

   return ::close(sock) == 0;
}


/* -------------------------------------------------------------------------- */

