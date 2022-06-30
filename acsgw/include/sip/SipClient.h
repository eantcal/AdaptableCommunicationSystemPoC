//
// This file is part of acsgw
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.
// Licensed under the MIT License.
// See COPYING file in the project root for full license information.
//


#pragma once

/* -------------------------------------------------------------------------- */

#include "LockedQueue.h"
#include "TransportSocket.h"
#include "SipParsedMessage.h"

#include <string>
#include <thread>
#include <atomic>
#include <memory>


/* -------------------------------------------------------------------------- */

class SipClient
{
public:
   using Handle=std::shared_ptr<SipClient>;

   SipClient(const std::string &remoteAddr,
             const TransportSocket::Port &remotePort,
             const std::string &localAddr = "",
             const TransportSocket::Port &localPort = 0);

   enum
   {
      SOCKET_RETRY_INTV = 5 /* sec */,
      BIND_RETRY_INTV = 5 /* sec */,
      CONNECT_RETRY_INTV = 5 /* sec */,
      RECV_TIMEOUT = 900 /* sec */,
      SEND_RECV_CLIENT_CONNECT_ATTEMPTS = 5,
      SEND_RECV_TIMEOUT = 900 /* sec, for debugging purposes for now it is 900 seconds */
   };

   SipParsedMessage::Handle sendrecv(std::string &&msg);
   virtual ~SipClient();

private:
   void _connectionMgr();

   std::string _remoteAddr;
   TransportSocket::Port _remotePort;
   std::string _localAddr;
   TransportSocket::Port _localPort;

   std::atomic_bool _connected{false};
   std::atomic_bool _stop{false};
   std::unique_ptr<std::thread> _connectionMgrHandle;

   LockedQueue<std::string> _sendMsg{1};
   LockedQueue<SipParsedMessage::Handle> _recvMsg{1};
};

