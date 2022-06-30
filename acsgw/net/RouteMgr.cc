#include "RouteMgr.h"
#include "Logger.h"

#include <sstream>
#include <string>
#include <stdlib.h>
#include <iostream>

/* -------------------------------------------------------------------------- */

bool RouteMgr::add(const IpAddress &ip, const std::string& dev, const std::string& mask)
{
    del(ip,mask); // remove any existing route
    
    // ip route add {NETWORK/MASK} via {GATEWAYIP}
    std::stringstream ss;
    ss << "ip route add " << ip.to_str() << mask << " dev " << dev;

    TRACE(LOG_DEBUG, "RouteMgr %s", ss.str().c_str());

    return ::system(ss.str().c_str()) == 0;
}

/* -------------------------------------------------------------------------- */

bool RouteMgr::add(const IpAddress &ip, const IpAddress &via, const std::string& mask)
{
    del(ip,mask); // remove any existing route

    // ip route add {NETWORK/MASK} dev {DEVICE}
    std::stringstream ss;
    ss << "ip route add " << ip.to_str() << mask << " via " << via.to_str();

    TRACE(LOG_DEBUG, "RouteMgr %s", ss.str().c_str());

    return ::system(ss.str().c_str()) == 0;
}

/* -------------------------------------------------------------------------- */

bool RouteMgr::del(const IpAddress &ip, const std::string& mask)
{
    std::stringstream ss;
    ss << "ip route del " << ip.to_str() << mask;

    TRACE(LOG_DEBUG, "RouteMgr %s", ss.str().c_str());

    return ::system(ss.str().c_str()) == 0;
}
