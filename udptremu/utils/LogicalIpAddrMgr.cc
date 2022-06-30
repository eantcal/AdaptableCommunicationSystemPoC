//
// This file is part of acsgw
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.
// Licensed under the MIT License.
// See COPYING file in the project root for full license information.
//

/* -------------------------------------------------------------------------- */

#include "LogicalIpAddrMgr.h"

/* -------------------------------------------------------------------------- */

bool LogicalIpAddrMgr::configure(const Config &config)
{
    std::lock_guard<std::mutex> guard(_lock);

    //We cannot configure twice
    if (_lastip.to_uint32() > 0)
    {
        return false;
    }

    //[logical_address_range]
    //first_ip = "10.0.0.1"
    //last_ip =  "10.255.255.254"

    const auto &cfgdata = config.data();

    // [logical_address_range]
    auto it = cfgdata.find("logical_address_range");

    if (it != cfgdata.end())
    {
        const auto &namespace_data = it->second;

        // first_ip = "x.x.x.x"
        auto nsit = namespace_data.find("first_ip");
        if (nsit != namespace_data.end())
        {
            _firstip = IpAddress(nsit->second.first); // if empty will bind to anyaddr
        }

        // first_ip = "x.x.x.x"
        nsit = namespace_data.find("last_ip");
        if (nsit != namespace_data.end())
        {
            _lastip = IpAddress(nsit->second.first); // if empty will bind to anyaddr
        }

        // ttl = N
        nsit = namespace_data.find("ttl");
        if (nsit != namespace_data.end())
        {
            try {
                _defaultTTL = std::stoul(nsit->second.first);
            }
            catch (...) {}
        }
    }

    return _firstip.to_uint32() > 0 && _lastip.to_uint32() >= _firstip.to_uint32();
}

LogicalIpAddrMgr &LogicalIpAddrMgr::getInstance()
{
    static LogicalIpAddrMgr _instance;
    return _instance;
}

/* -------------------------------------------------------------------------- */

bool LogicalIpAddrMgr::resolveIp(const std::string& name, IpAddress &ip)
{
    std::lock_guard<std::mutex> guard(_lock);

    // Resolve IP by name
    auto it = _name2ipTbl.find(name);

    if (it != _name2ipTbl.end()) 
    {
        ip = it->second;
        return true;
    }

    return false;
}

/* -------------------------------------------------------------------------- */

bool LogicalIpAddrMgr::generateIp(const std::string& name, IpAddress &ip)
{
    std::lock_guard<std::mutex> guard(_lock);

    // Resolve IP by name
    auto it = _name2ipTbl.find(name);

    if (it != _name2ipTbl.end()) 
    {
        ip = it->second;
        return true;
    }

    auto absTimeMs = _timeSinceEpochMillisec();
    auto expiringAt = absTimeMs + 1000UL * _defaultTTL;

    // If no IPs have been assigned, take the first one //
    if (_ipByTime.empty())
    {
        _ipByTime.insert({expiringAt, _firstip});
        _ipByAddr.insert({_firstip, expiringAt});
        ip = _firstip;
        _name2ipTbl[name] = ip;
        return true;
    }

    auto evicting_ip_it = _ipByTime.begin();

    // Find out if there is any assigned IP can be evicted (since TTL expired) //
    if (evicting_ip_it->first < absTimeMs)
    {
        auto evicting_ip = evicting_ip_it->second;
        _ipByTime.erase(evicting_ip_it);
        _ipByAddr.erase(evicting_ip);
        _ipByTime.insert({expiringAt, evicting_ip});
        _ipByAddr.insert({evicting_ip, expiringAt});
        ip = evicting_ip;
        _name2ipTbl[name] = ip;
        return true;
    }

    // Allocate any new available IP //
    auto last_recent_inserted = _ipByTime.rbegin();

    // Check if the IP interval is exausted
    if (last_recent_inserted->second.to_uint32() >= _lastip.to_uint32())
        return false;

    // Search for any available IP
    auto nextIp = IpAddress(last_recent_inserted->second.to_uint32() + 1);
    while (_ipByAddr.find(nextIp) != _ipByAddr.end())
    {
        if (nextIp.to_uint32() >= _lastip.to_uint32())
            return false;

        // In case the IP was assigned, check next one
        nextIp = IpAddress(nextIp.to_uint32() + 1);
    }

    // Allocate the new IP
    _ipByTime.insert({expiringAt, nextIp});
    _ipByAddr.insert({nextIp, expiringAt});

    ip = nextIp;
    _name2ipTbl[name] = ip;
    return true;
}

/* -------------------------------------------------------------------------- */

std::string LogicalIpAddrMgr::getHosts() const noexcept
{
    std::string ret;

    for (const auto& [host, ip] : _name2ipTbl) 
    {
        ret += ip.to_str();
        ret += "     ";
        ret += host;
        ret += "\n";
    }

    return ret;
}