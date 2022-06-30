//
// This file is part of acsgw
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.
// Licensed under the MIT License.
// See COPYING file in the project root for full license information.
//

/* -------------------------------------------------------------------------- */

#include "MpTunnel.h"
#include "VirtualIfMgr.h"
#include "GreSocket.h"
#include "TcpSocket.h"
#include "UdpSocket.h"
#include "Logger.h"
#include "TcpListener.h"
#include "IpPacketParser.h"

#include <cassert>
#include <string>
#include <sstream>
#include <map>
#include <memory>

/* -------------------------------------------------------------------------- */

bool TunnelPath::makeGreSocket() noexcept
{
   _greSocket = std::make_unique<GreSocket>();

   assert(_greSocket);

   if (!_greSocket || !_greSocket->isValid())
   {
      TRACE(LOG_ERR, "%s failed due an invalid socket", __FUNCTION__);

      return false;
   }

   if (!_greSocket->bind(_localAddr, true /* reuse addr */))
   {
      TRACE(LOG_ERR, "%s bind(%s) failed with error: '%s'", __FUNCTION__,
            _localAddr.to_str().c_str(), strerror(errno));

      return false;
   }

   return true;
}

/* -------------------------------------------------------------------------- */

bool TunnelPath::makeTcpConnection() noexcept
{
   TRACE(LOG_DEBUG, "%s creating TCP connection for endpoint %s:%i", __FUNCTION__,
            _localAddr.to_str().c_str(), _localPort); 

   if (ConnRoleType::Client == getConnRole())
   {
      TRACE(LOG_DEBUG, "%s creating client connection %s:%i->%s:%i", __FUNCTION__,
            _localAddr.to_str().c_str(), _localPort, 
            _remoteAddr.to_str().c_str(), _remotePort); 

      _tcpConnectionMgr = std::make_shared<TcpConnectionMgr>(_localAddr, _localPort, _remoteAddr, _remotePort);
   }
   else
   {
      TRACE(LOG_DEBUG, ">>> %s creating server listener for %s:%i", __FUNCTION__,
            _localAddr.to_str().c_str(), _localPort);

      _tcpConnectionMgr = std::make_shared<TcpConnectionMgr>(_localAddr, _localPort);
   }

   return _tcpConnectionMgr->run();
}

/* -------------------------------------------------------------------------- */

bool TunnelPath::makeUdpSocket() noexcept
{
   _udpSocket = std::make_unique<UdpSocket>();

   assert(_udpSocket);

   if (!_udpSocket || !_udpSocket->isValid())
   {
      TRACE(LOG_ERR, "%s failed due an invalid socket", __FUNCTION__);

      return false;
   }

   if (!_udpSocket->bind(_localPort, _localAddr, true /* reuse addr */))
   {
      TRACE(LOG_ERR, "%s bind(%s) failed with error: '%s'", __FUNCTION__,
            _localAddr.to_str().c_str(), strerror(errno));

      return false;
   }

   return true;
}

/* -------------------------------------------------------------------------- */

TunnelPath::TunnelPath(const TunnelPath::Bearer &tp) : _localAddr(tp.localAddress()),
                                                       _remoteAddr(tp.remoteAddr()),
                                                       _localPort(tp.localPort()),
                                                       _remotePort(tp.remotePort())
{
}

/* -------------------------------------------------------------------------- */

int MpTunnelMgr::tunnelRecvThreadFunc(
    const std::string &name,
    MpTunnelMgr *tmPtr,
    std::shared_ptr<VirtualIfMgr> vifPtr,
    TunnelPath::Handle tpPtr)
{
   assert(tmPtr);
   assert(vifPtr);

   try
   {
      TunnelPath &tp = *tpPtr;

      // Manager will wait for unloking the mutex
      // while thread is terminanting
      struct tunnelLockGrd
      {
         TunnelPath &_t;

         tunnelLockGrd(TunnelPath &t) : _t(t)
         {
            _t.lock();
         }

         ~tunnelLockGrd() { _t.unlock(); }
      } grd(tp);

      char buf[VirtualIfMgr::MAX_PKT_SIZE] = {0};

      // TODO improve this
      static Ip4DupDetector ip4DupDetector;

      while (true)
      {
         if (tp.removeReqPending())
         {
            return 0;
         }

         struct timeval timeout = {0};
         timeout.tv_usec = 0;
         timeout.tv_sec = 5;

         TRACE(LOG_NOTICE, "%s %s", __FUNCTION__, "Waiting for packets");
         int payloadOffset = 0;
         IpAddress remoteAddr;
         size_t rbytescnt = 0;
         int socketType = 0;

         if (tp.getGreSocket())
         {
            GreSocket::PollingState pollst = tp.getGreSocket()->poll(timeout);

            if (pollst == GreSocket::PollingState::TIMEOUT_EXPIRED)
               continue;
            else if (pollst == GreSocket::PollingState::ERROR_IN_COMMUNICATION)
               return -1;

            payloadOffset = 0;

            rbytescnt = tp.getGreSocket()->recvfrom(buf, sizeof(buf), remoteAddr, payloadOffset);

            socketType = 1;
         }
         else if (tp.getUdpSocket())
         {
            UdpSocket::PollingState pollst = tp.getUdpSocket()->poll(timeout);

            if (pollst == UdpSocket::PollingState::TIMEOUT_EXPIRED)
               continue;
            else if (pollst == UdpSocket::PollingState::ERROR_IN_COMMUNICATION)
               return -1;

            payloadOffset = 0; // no additional header for now
            UdpSocket::PortType remotePort = 0;

            rbytescnt = tp.getUdpSocket()->recvfrom(buf, sizeof(buf), remoteAddr, remotePort);

            socketType = 2;
         }
         else if (tp.getTcpConnMgr())
         {
            TcpConnectionMgr::Buffer msg;
            if (!tp.getTcpConnMgr()->recvMessage(msg, timeout.tv_sec * 1000)) {
               // timeout expired
               continue;           
            }
            
            rbytescnt = msg.size();
            
            if (msg.size()>0) {
               memcpy(buf, msg.data(), rbytescnt);
            }
            
            payloadOffset = 0; // no additional header for now

            socketType = 3;
         }
         else
         {
            TRACE(LOG_ERR,
            "%s TCP connection invalid, (tpPtr=%p) "
            "related to the tunnel for ndd %s",
            __FUNCTION__, tpPtr, name.c_str());
            return -1;      
         }

         if (rbytescnt > 0)
         {
            uint64_t pktid = 0;
            
            if (socketType >1 ) 
            {
               memcpy((char*) &pktid, buf+rbytescnt-sizeof(sizeof(pktid)), sizeof(pktid));
               rbytescnt -= sizeof(pktid);
            }
            
            IpPacketParser ipParser(buf + payloadOffset, rbytescnt);

            ipParser.dump(std::cout);

            const auto packetDuplicated = (socketType<2 && ipParser.isIcmp() && ip4DupDetector.isADuplicated(ipParser)) ||
                                          socketType>1 && ip4DupDetector.isADuplicated(pktid);

            /// DEBUG ONLY
            /// std::stringstream ss;
            /// ss << pktid << std::endl;
            /// auto pktids = ss.str();
            /// TRACE(LOG_NOTICE, "**** %s PACKET id=%08x from %s to ndd %s (pktid=%s)",
            ///         __FUNCTION__, ipParser.getIdent(), std::string(remoteAddr).c_str(), name.c_str(), pktids.c_str());

            if (packetDuplicated)
            {
               TRACE(LOG_NOTICE, "%s discarded DUP PACKET id=%08x from %s to ndd %s",
                     __FUNCTION__, ipParser.getIdent(), std::string(remoteAddr).c_str(), name.c_str());
            }
            else {
               vifPtr->announcePacket(name.c_str(), buf + payloadOffset, rbytescnt);

               TRACE(LOG_NOTICE, "%s announced packet from %s to ndd %s",
                     __FUNCTION__, std::string(remoteAddr).c_str(), name.c_str());
            }
         }
         else if (rbytescnt == 0)
         {
            TRACE(LOG_ERR,
                  "%s timeout (it's ok) & continue while NO packets in this (timeout) interval from "
                  "tunnel for ndd %s",
                  __FUNCTION__, name.c_str());
         }
         else
         {
            TRACE(LOG_ERR,
                  "%s failed receiving packet from "
                  "tunnel for ndd %s",
                  __FUNCTION__, name.c_str());

            return -1;
         }
      }
   }
   catch (Exception)
   {
      TRACE(LOG_ERR,
         "%s detected an unmanaged exception handling the tunnel (tpPtr=%p) for ndd %s",
         __FUNCTION__, tpPtr, name.c_str());
      return -1;
   }
   catch (...)
   {
      TRACE(LOG_ERR,
         "%s detected an unknown exception handling the tunnel (tpPtr=%p) for ndd %s",
         __FUNCTION__, tpPtr, name.c_str());
      assert(0);
   }

   return 0;
}

/* -------------------------------------------------------------------------- */

int MpTunnelMgr::tunnelXmitThreadFunc(
    MpTunnelMgr *tmPtr,
    std::shared_ptr<VirtualIfMgr> vifPtr)
{
   assert(tmPtr);
   assert(vifPtr);

   std::string if_name;

   uint64_t pktid = 0;

   try
   {
      while (true)
      {
         char buf[VirtualIfMgr::MAX_PKT_SIZE + 8] = {0};

         // GRE Header with protocol type 0x0800
         *(uint16_t *)(buf) = 0;
         *(uint16_t *)(buf + 2) = htons(0x0800);

         constexpr int GRE_HEADER_LEN=4;

         // Get a new packet from tun/tap driver
         const size_t buflen =
             vifPtr->getPacket(buf + GRE_HEADER_LEN, sizeof(buf) - GRE_HEADER_LEN - 8, if_name);

         
         ++pktid;

         if (buflen > 0 && buflen <= sizeof(buf))
         {
            try
            {
               auto mpTunnel = tmPtr->getMpTunnel(if_name);

               // send the packet on each bearer of multi-path tunnel
               for (auto tpPtr : mpTunnel)
               {
                  TunnelPath &tp = *tpPtr;
                  const auto remoteAddr = tp.getRemoteIp();
                  const auto remotePort = tp.remotePort();

                  if (tp.getGreSocket())
                  {
                     TRACE(LOG_NOTICE, "%s: receiving packet from ndd %s, tx to %s",
                           __FUNCTION__, if_name.c_str(),
                           std::string(remoteAddr).c_str());

                     size_t bsent = tp.getGreSocket()->sendto(buf, buflen + GRE_HEADER_LEN, remoteAddr);

                     if (bsent <= 0)
                     {
                        TRACE(LOG_ERR, "%s: tunnel.getTunnelSocket().sendto "
                                       "error sending sending to %s",
                              __FUNCTION__, std::string(remoteAddr).c_str());
                        return -1;
                     } //..if
                  }
                  else if (tp.getUdpSocket())
                  {
                     TRACE(LOG_NOTICE, "%s: receiving packet from ndd %s, tx to %s:%i",
                           __FUNCTION__, if_name.c_str(),
                           std::string(remoteAddr).c_str(),
                           remotePort);

                     memcpy(buf + GRE_HEADER_LEN + buflen, (const char*) &pktid, sizeof(pktid));

                     // Skip GRE Header bytes here
                     size_t bsent = tp.getUdpSocket()->sendto(buf + GRE_HEADER_LEN, buflen + sizeof(pktid), remoteAddr, remotePort);

                     if (bsent <= 0)
                     {
                        TRACE(LOG_ERR, "%s: tunnel.getUdpSocket().sendto "
                                       "error sending sending to %s",
                              __FUNCTION__, std::string(remoteAddr).c_str());
                        return -1;
                     } //..if
                  }
                  else if (tp.getTcpConnMgr())
                  {
                     // Reserve first 4 bytes for TCP segment len
                     TcpConnectionMgr::Buffer msg(&buf[0], &buf[buflen + 4 + sizeof(pktid)]);

                     // Put pktid at the end of message (following the payload)
                     memcpy((char*) (&(msg.front()))+4+buflen, (const char*) &pktid, sizeof(pktid));

                     const bool res = tp.getTcpConnMgr()->sendMessage(std::move(msg));
                     if (!res)
                     {
                        TRACE(LOG_ERR, "%s: tunnel.getUdpSocket().sendMessage "
                                       "error sending a message to %s (TX queue full?)",
                              __FUNCTION__, std::string(remoteAddr).c_str());
                        continue;
                     } //..if
                  }
               }
            }
            catch (MpTunnelMgr::Exception &e)
            {
               switch (e)
               {
               case MpTunnelMgr::Exception::TUNNEL_INSTANCE_NOT_FOUND:
                  TRACE(LOG_WARNING, "%s: tunnel instance not found",
                        __FUNCTION__);
                  break;
               default:
                  TRACE(LOG_ERR, "%s: failing due to an unexpected exception [1]",
                        __FUNCTION__);
                  assert(0);
               }
            } //...catch
         }
      } // ... while (1)
   }
   catch (...)
   {
      TRACE(LOG_ERR, "%s: failing due to an unexpected exception [2]",
            __FUNCTION__);
      assert(0);
   }

   return 0;
}

/* -------------------------------------------------------------------------- */

bool MpTunnelMgr::addBearer(
    const std::string &ifname,
    const TunnelPath::Bearer &bearer,
    std::shared_ptr<VirtualIfMgr> vifPtr)
{
   TRACE(LOG_NOTICE, "%s adds new bearer (%08x-%08x) to '%s'",
         __FUNCTION__,
         (unsigned)bearer.localAddress(), (unsigned)bearer.remoteAddr(),
         ifname.c_str());

   assert(vifPtr);
   lock_guard_t with(_lock);

   TunnelPath::Handle tpPtr;

   try
   {
      tpPtr.reset(new TunnelPath(bearer));
   }
   catch (TunnelPath::Exception)
   {
      TRACE(LOG_WARNING, "%s cannot assign to '%s' the path %s->%s",
            __FUNCTION__, ifname.c_str(),
            bearer.localAddress().to_str().c_str(),
            bearer.remoteAddr().to_str().c_str());
      return false;
   }

   assert(tpPtr);

   switch (bearer.getTunnelProtocol())
   {
   case TunnelProtocol::Tcp:
      TRACE(LOG_WARNING, ">>>>>> %s creating TCP session for bearer '%s'", __FUNCTION__, ifname.c_str());
      if (!tpPtr->makeTcpConnection())
      {
         TRACE(LOG_WARNING, "%s cannot create TCP session for bearer '%s'", __FUNCTION__, ifname.c_str());
         return false;
      }
      break;

   case TunnelProtocol::Udp:
      TRACE(LOG_WARNING, "%s creating UDP socket for bearer '%s'", __FUNCTION__, ifname.c_str());
      if (!tpPtr->makeUdpSocket())
      {
         TRACE(LOG_WARNING, "%s cannot create UDP socket for bearer '%s'", __FUNCTION__, ifname.c_str());
         return false;
      }
      break;

   case TunnelProtocol::Gre:
   default:
      TRACE(LOG_WARNING, "%s creating GRE socket for bearer '%s'", __FUNCTION__, ifname.c_str());
      if (!tpPtr->makeGreSocket())
      {
         TRACE(LOG_WARNING, "%s cannot create GRE socket for bearer '%s'", __FUNCTION__, ifname.c_str());
         return false;
      }
      break;
   }

   if (vifPtr->addIf(ifname) < 0)
   {
      TRACE(LOG_WARNING, "%s cannot add i/f '%s'", __FUNCTION__, ifname.c_str());
      return false;
   }

   if (!_rpeer2dev.insert(
                      std::make_pair(bearer.remoteAddr().to_uint32(), ifname))
            .second)
   {
      TRACE(LOG_WARNING, "%s cannot register i/f '%s' with id=%08x",
            __FUNCTION__, ifname.c_str(), bearer.remoteAddr().to_uint32());
      return false;
   }

   auto it = _dev2mpTunnel.find(ifname);

   if (it != _dev2mpTunnel.end())
   {
      it->second.push_back(tpPtr);
   }
   else
   {
      MpTunnel tg{tpPtr};
      _dev2mpTunnel.insert({ifname, std::move(tg)});
   }

   // There is a single tx thread for all the tunnels
   if (_tunnelXmitThreadObj == nullptr)
   {
      _tunnelXmitThreadObj.reset(
          new std::thread(
              &MpTunnelMgr::tunnelXmitThreadFunc,
              this,
              vifPtr));

      if (_tunnelXmitThreadObj == nullptr)
      {
         // clean up allocated resources
         _dev2mpTunnel.erase(ifname);
         _rpeer2dev.erase(bearer.remoteAddr().to_uint32());
         return false;
      }
   }

   // Create a receiver thread for this tunnel instance
   std::thread recv_thread(
       &MpTunnelMgr::tunnelRecvThreadFunc,
       ifname, this, vifPtr, tpPtr);

   recv_thread.detach();

   return true;
}

/* -------------------------------------------------------------------------- */

MpTunnelMgr::MpTunnel MpTunnelMgr::getMpTunnel(const std::string &ifname)
{
   lock_guard_t with(_lock);

   Dev2MpTunnelLookupTbl ::iterator i = _dev2mpTunnel.find(ifname);

   if (i == _dev2mpTunnel.end())
   {
      TRACE(LOG_WARNING, "%s cannot find i/f '%s'", __FUNCTION__, ifname.c_str());
      throw Exception::TUNNEL_INSTANCE_NOT_FOUND;
   }

   return i->second;
}

/* -------------------------------------------------------------------------- */

bool MpTunnelMgr::delMpTunnel(const std::string &ifname) noexcept
{
   lock_guard_t with(_lock);

   Dev2MpTunnelLookupTbl ::const_iterator mpTunnel = _dev2mpTunnel.find(ifname);

   if (mpTunnel != _dev2mpTunnel.end())
   {
      for (auto &tunnelInstance : mpTunnel->second)
      {
         _rpeer2dev.erase(tunnelInstance->getRemoteIp().to_uint32());

         // notify to the receiver thread that
         // we are going to delete the tunnel instance
         tunnelInstance->notifyRemoveReqPending();

         // wait until recv thread terminates execution
         tunnelInstance->lock();

         //remove the tunnel instance
         _dev2mpTunnel.erase(ifname);

         tunnelInstance->unlock(); // reset lock counter
      }
      return true;
   }

   TRACE(LOG_WARNING, "%s cannot find i/f '%s'", __FUNCTION__, ifname.c_str());
   return false;
}
