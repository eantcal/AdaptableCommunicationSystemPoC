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

#include "Config.h"
#include "IpAddress.h"

#include <map>
#include <mutex>
#include <chrono>

/* -------------------------------------------------------------------------- */

class LogicalIpAddrMgr
{
   LogicalIpAddrMgr() = default;
   LogicalIpAddrMgr(const LogicalIpAddrMgr &) = delete;
   LogicalIpAddrMgr &operator=(const LogicalIpAddrMgr &) = delete;

public:
   static LogicalIpAddrMgr &getInstance();

   bool configure(const Config &config);
   bool generateIp(const std::string& name, IpAddress &ip);
   bool resolveIp(const std::string& name, IpAddress &ip);

   std::string getHosts() const noexcept;

private:
   IpAddress _firstip;
   IpAddress _lastip;
   std::multimap<uint64_t, IpAddress> _ipByTime;
   std::map<IpAddress, uint32_t> _ipByAddr;
   std::unordered_map<std::string, IpAddress> _name2ipTbl;
   std::mutex _lock;
   uint64_t _defaultTTL = 3600; //sec

   static uint64_t _timeSinceEpochMillisec()
   {
      using namespace std::chrono;
      return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
   }
};
