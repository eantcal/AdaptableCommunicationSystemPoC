//
// This file is part of acsgw
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.
// Licensed under the MIT License.
// See COPYING file in the project root for full license information.
//

/* -------------------------------------------------------------------------- */

#include "TunnelBuilder.h"
#include "Logger.h"

/*
Example of a valid configuration file content

############################ Tunnels configuration ############################
#
[tunnels]
list = "tunnel1, tunnel2" # list of tunnels to create

# Defines the bearer used by tunnels
[bearer1]
local_address ="192.168.0.73"
remote_address="192.168.0.46"

[bearer2]
local_address ="192.168.0.73"  # TBD
remote_address="192.168.0.46" # TBD

# Defines the tunnels
[tunnel1]
bearers       ="bearer1"    # List of bearers used to create a tunnel
type          ="gre"        # Tunnelling protocol
local_address ="10.0.0.1"   # Logical address of tunnel ingress
remote_address="10.0.0.2"   # Logical address of tunnel egress

[tunnel2]
bearers        ="bearer1, bearer2" # Multiple baerers tunnel
type           ="gre"              
local_address  ="10.0.0.3"
remote_address ="10.0.0.4"
multipath      ="mirroring" # Defines the algo used in case of multiple paths*/

/* -------------------------------------------------------------------------- */

TunnelBuilder::TunnelBuilder(Config &cfg)
{
    _tunnels = parseCfg(cfg);
    _vifmgr = std::make_shared<VirtualIfMgr>();
}

/* -------------------------------------------------------------------------- */

bool TunnelBuilder::buildTunnels()
{
    bool ret = true;

    for (const auto &tunnel : *_tunnels)
    {
        if (!createTunnel(tunnel.first, tunnel.second))
        {
            ret = false;
        }
    }

    return ret;
}

/* -------------------------------------------------------------------------- */

bool TunnelBuilder::createTunnel(const std::string &ifname, const Tunnel &tunnel)
{
    //sudo ip tunnel add <ifname> mode <type> remote <remoteaddr> local <localaddr> ttl 255
    if (tunnel.bearers.empty())
    {
        std::cerr << "No bearers to create tunnels" << std::endl;
        return false;
    }

    // Remove any Kernel-managed old instance of this tunnel
    std::string cmd{"ip tunnel del "};
    cmd += ifname;
    auto ret = ::system(cmd.c_str());
    (void) ret;
    // If the tunnel has just one bearer and it is GRE, try to create a Kernel managed GRE tunnel
    if (tunnel.bearers.size() == 1 && tunnel.bearers.begin()->tunnelProtocol == TunnelProtocol::Gre)
    {
        TRACE(LOG_DEBUG, "%s %s", __FUNCTION__, cmd.c_str());

        auto bearer = *tunnel.bearers.begin();

        cmd =
            "ip tunnel add " +
            ifname +
            " mode " + tunnel.type +
            " remote " + bearer.remoteAddress +
            " local " + bearer.localAddress +
            " ttl 255";

        TRACE(LOG_DEBUG, "%s %s", __FUNCTION__, cmd.c_str());

        if (::system(cmd.c_str()) == 0)
        {
            cmd =
                "ifconfig " +
                ifname +
                " " + tunnel.localAddress;

            TRACE(LOG_DEBUG, "%s %s", __FUNCTION__, cmd.c_str());

            if (::system(cmd.c_str()) == 0)
            {
                cmd =
                  "route add " +
                  tunnel.remoteAddress +
                  " dev " + ifname;

                  TRACE(LOG_DEBUG, "%s %s", __FUNCTION__, cmd.c_str());

                  if (::system(cmd.c_str()) == 0) {
                      return true;
                  }
            }
        }

        // If we weren't able to use Kernel GRE module, just retry by managing the tunnel via TM
    }

    TRACE(LOG_DEBUG, ">>>> %s adding %i bearers to '%s'",
            __FUNCTION__,
            tunnel.bearers.size(),
            ifname.c_str());

    for (auto bearer : tunnel.bearers) {
        TRACE(LOG_DEBUG, ">>>>> %s going to add a bearer (%s-%s) to '%s'",
                      __FUNCTION__,
                      bearer.localAddress.c_str(),
                      bearer.remoteAddress.c_str(),
                      ifname.c_str());
    }

    for (auto bearer : tunnel.bearers)
    {
        try
        {
            if (!_mpTunnelMgr.addBearer(
                    ifname,
                    TunnelPath::Bearer(
                        IpAddress(bearer.localAddress),
                        IpAddress(bearer.remoteAddress),
                        bearer.port,
                        bearer.port,
                        bearer.tunnelProtocol),
                    _vifmgr))
            {
                TRACE(LOG_WARNING, "%s cannot add a bearer (%s-%s) to '%s'",
                      __FUNCTION__,
                      bearer.localAddress.c_str(),
                      bearer.remoteAddress.c_str(),
                      ifname.c_str());
            }
            else {
                cmd =
                    "ifconfig " +
                    ifname +
                    " " + tunnel.localAddress;

                TRACE(LOG_DEBUG, "%s %s", __FUNCTION__, cmd.c_str());

                if (::system(cmd.c_str()) == 0)
                {
                    cmd =
                        "route add " +
                        tunnel.remoteAddress +
                        " dev " + ifname;

                    TRACE(LOG_DEBUG, "%s %s", __FUNCTION__, cmd.c_str());

                    if (::system(cmd.c_str()) != 0)
                    {
                        TRACE(LOG_DEBUG, "%s could not run %s", __FUNCTION__, cmd.c_str());
                    }
                }
            }
        }
        catch (MpTunnelMgr::Exception &e)
        {
            TRACE(LOG_WARNING, "%s cannot add multiple tunnel for i/f '%s'", __FUNCTION__, ifname.c_str());
            return false;
        }
    }

    return true;

}

/* -------------------------------------------------------------------------- */

TunnelBuilder::LookupTblHandler TunnelBuilder::parseCfg(const Config &cfg)
{
    LookupTbl tunnel_map;

    cfg.selectNameSpace("tunnels");
    auto listOfTunnels = cfg.getAttrList("list");

    auto getNum = [&](
                      const Config::ConfigNamespacedParagraph &nsp,
                      const std::string &fieldName,
                      int minVal, int maxVal,
                      uint16_t &retVal)
    {
        auto nsit = nsp.find(fieldName);
        if (nsit != nsp.end())
        {
            try
            {
                int n = std::stoi(nsit->second.first);
                if (n >= minVal && n <= maxVal)
                {
                    retVal = uint16_t(n);
                }
            }
            catch (...)
            {
                TRACE(LOG_DEBUG, "TunnelBuilder config syntax error in %s format", fieldName.c_str());
            }
        }
    };

    for (const auto &tunnel : listOfTunnels)
    {
        auto it = cfg.data().find(tunnel);
        uint16_t nPort = 28774; // default

        if (it != cfg.data().end())
        {
            const auto &namespace_data = it->second;
            getNum(namespace_data, "port", 1, 65535, nPort);
        }

        cfg.selectNameSpace(tunnel);
        Tunnel &tunnel_data = tunnel_map[tunnel];
        tunnel_data.localAddress = cfg.getAttr("local_address");
        tunnel_data.remoteAddress = cfg.getAttr("remote_address");
        const auto &type = cfg.getAttr("type");
        tunnel_data.type = type.empty() ? "gre" : type; // if type is not def set gre
        auto bearers = cfg.getAttrList("bearers");

        tunnel_data.port = nPort;

        for (const auto &bearer : bearers)
        {
            cfg.selectNameSpace(bearer);
            Bearer bearer_data;
            bearer_data.localAddress = cfg.getAttr("local_address");
            bearer_data.remoteAddress = cfg.getAttr("remote_address");

            auto bearer_type = cfg.getAttr("type");

            const auto protocol_type = !bearer_type.empty() ? bearer_type : tunnel_data.type;

            if (protocol_type == "tcp")
            {
                bearer_data.tunnelProtocol = TunnelProtocol::Tcp;
            }
            else if (protocol_type == "udp")
            {
                bearer_data.tunnelProtocol = TunnelProtocol::Udp;
            }
            else
            {
                bearer_data.tunnelProtocol = TunnelProtocol::Gre;
            }

            bearer_data.port = nPort;

            tunnel_data.bearers.push_back(std::move(bearer_data));
        }
    }

    return std::make_shared<LookupTbl>(std::move(tunnel_map));
}
