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

#include "GreSocket.h"
#include "UdpSocket.h"
#include "TcpSocket.h"
#include "IpAddress.h"
#include "VirtualIfMgr.h"
#include "TcpConnectionMgr.h"
#include "IpPacketParser.h"

#include <thread>
#include <mutex>
#include <string>
#include <sstream>
#include <map>
#include <list>
#include <memory>

/* -------------------------------------------------------------------------- */

enum class TunnelProtocol {
   Gre,
   Udp,
   Tcp
};

class TunnelPath
{
public:
   using Handle = std::shared_ptr<TunnelPath>;
   using TcpConnMgrPtr = std::shared_ptr<TcpConnectionMgr>;
   using GreSocketPtr = std::shared_ptr<GreSocket>;
   using UdpSocketPtr = std::shared_ptr<UdpSocket>;

   enum class Exception
   {
      BINDING_SOCKET_ERROR,
      INVALID_SOCKET_ERROR
   };

   struct Bearer
   {
   private:
      IpAddress _localAddr;
      IpAddress _remoteAddr;
      int _localPort = -1;
      int _remotePort = -1;
      TunnelProtocol _tunnelProtocol = TunnelProtocol::Gre;

      Bearer() = delete;

   public:
      Bearer(const Bearer &) = default;
      Bearer &operator=(const Bearer &) = default;

      const IpAddress &localAddress() const noexcept
      {
         return _localAddr;
      }

      const IpAddress &remoteAddr() const noexcept
      {
         return _remoteAddr;
      }

      const int &localPort() const noexcept
      {
         return _localPort;
      }

      const int &remotePort() const noexcept
      {
         return _remotePort;
      }

      const TunnelProtocol& getTunnelProtocol() const 
      {
         return _tunnelProtocol;
      }

      explicit inline Bearer(const IpAddress &lip, 
                             const IpAddress &rip,
                             int localPort,
                             int remotePort,
                             const TunnelProtocol& protocol) : 
          _localAddr(lip),
          _remoteAddr(rip),
          _localPort(localPort),
          _remotePort(remotePort),
          _tunnelProtocol(protocol)
      {
      }

      operator std::string() const noexcept
      {
         std::stringstream os;

         if (getTunnelProtocol() == TunnelProtocol::Gre) {
            os << _localAddr.to_str().c_str() << "<->"
               << _remoteAddr.to_str().c_str();
         }
         else {
            os << _localAddr.to_str().c_str() << ":" << _localPort << "<->"
               << _remoteAddr.to_str().c_str() << ":" << _remotePort;
         }

         return os.str();
      }

      
   };

   IpAddress getLocalIp() const noexcept
   {
      return _localAddr;
   }

   IpAddress getRemoteIp() const noexcept
   {
      return _remoteAddr;
   }

   const uint16_t &localPort() const noexcept
   {
      return _localPort;
   }

   const uint16_t &remotePort() const noexcept
   {
      return _remotePort;
   }

   bool removeReqPending() const noexcept
   {
      return _remove_req_pending;
   }

   void notifyRemoveReqPending() noexcept
   {
      _remove_req_pending = true;
   }

   void lock() noexcept
   {
      _lock.lock();
   }

   void unlock() noexcept
   {
      _lock.unlock();
   }

   bool tryLock() noexcept
   {
      return _lock.try_lock();
   }

   TunnelPath(const Bearer &tp);

   friend std::stringstream &operator<<(
       std::stringstream &os, const TunnelPath &tunnel)
   {
      return os << std::string(tunnel), os;
   }

   bool equals(const TunnelPath &obj) const noexcept
   {
      return _localAddr == obj._localAddr &&
             _remoteAddr == obj._remoteAddr;
   }

   bool operator==(const TunnelPath &obj) const noexcept
   {
      return equals(obj);
   }

   bool operator!=(const TunnelPath &obj) const noexcept
   {
      return !equals(obj);
   }

   bool makeGreSocket() noexcept;
   bool makeUdpSocket() noexcept;
   bool makeTcpConnection() noexcept;

   GreSocketPtr getGreSocket() const noexcept { return _greSocket; }
   UdpSocketPtr getUdpSocket() const noexcept { return _udpSocket; }
   TcpConnMgrPtr getTcpConnMgr() const noexcept { return _tcpConnectionMgr; }

   enum class ConnRoleType {
      Client,
      Server
   };

   ConnRoleType getConnRole() const noexcept
   {
      const bool client = _localAddr < _remoteAddr || (_localAddr == _remoteAddr && _localPort < _remotePort);
      return client ? ConnRoleType::Client : ConnRoleType::Server;
   }

   operator std::string() const noexcept
   {
      std::stringstream os;

      os << _localAddr.to_str().c_str() << "<->"
         << _remoteAddr.to_str().c_str();

      return os.str();
   }

private:
   IpAddress _localAddr;
   IpAddress _remoteAddr;
   uint16_t _localPort = 0;
   uint16_t _remotePort = 0;

   GreSocketPtr _greSocket;
   UdpSocketPtr _udpSocket;
   TcpConnMgrPtr _tcpConnectionMgr;

   mutable std::recursive_mutex _lock;
   using lock_guard_t = std::lock_guard<std::recursive_mutex>;

   bool _remove_req_pending = false;

   //void _bindLocalAddr();

   TunnelPath() = delete;
   TunnelPath(const TunnelPath &obj) = delete;
   TunnelPath &operator=(const TunnelPath &obj) = delete;
};

/* -------------------------------------------------------------------------- */

class MpTunnelMgr
{
public:
   enum class Exception
   {
      TUNNEL_INSTANCE_NOT_FOUND
   };

private:
   using lock_guard_t = std::lock_guard<std::recursive_mutex>;

   using MpTunnel = std::list<TunnelPath::Handle>;
   using Dev2MpTunnelLookupTbl = std::map<std::string, MpTunnel>;
   using Remote2DevLookupTbl = std::map<uint32_t, std::string>;
   using ThreadHandle = std::unique_ptr<std::thread>;

   mutable std::recursive_mutex _lock;
   ThreadHandle _tunnelXmitThreadObj;
   Dev2MpTunnelLookupTbl _dev2mpTunnel;
   Remote2DevLookupTbl _rpeer2dev;

   static int tunnelRecvThreadFunc(
       const std::string &name_,
       MpTunnelMgr *tvm_,
       std::shared_ptr<VirtualIfMgr> vifPtr,
       TunnelPath::Handle tunnelHandle);

   static int tunnelXmitThreadFunc(
       MpTunnelMgr *tvm_,
       std::shared_ptr<VirtualIfMgr> vifPtr);

   MpTunnelMgr(const MpTunnelMgr &) = delete;
   MpTunnelMgr &operator=(const MpTunnelMgr &) = delete;

public:
   MpTunnelMgr() = default;

   ~MpTunnelMgr()
   {
      if (_tunnelXmitThreadObj)
         _tunnelXmitThreadObj->join();
   }

   size_t size() const noexcept
   {
      lock_guard_t cs(_lock);

      return _dev2mpTunnel.size();
   }

   bool empty() const noexcept
   {
      return size() == 0;
   }

   bool addBearer(
       const std::string &ifname,
       const TunnelPath::Bearer &tp,
       std::shared_ptr<VirtualIfMgr> vifPtr);

   bool tunnelExists(const std::string &ifname) const noexcept
   {
      lock_guard_t cs(_lock);

      return _dev2mpTunnel.find(ifname) != _dev2mpTunnel.end();
   }

   MpTunnel getMpTunnel(const std::string &ifname);

   bool delMpTunnel(const std::string &ifname) noexcept;

   friend std::stringstream &operator<<(
       std::stringstream &os,
       const MpTunnelMgr &vtm)
   {
      lock_guard_t cs(vtm._lock);

      for (const auto &e : vtm._dev2mpTunnel)
      {
         for (const auto &t : e.second)
         {
            os << e.first << " " << std::string(*t) << std::endl;
         }
      }

      return os;
   }
};
