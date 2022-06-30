//
// This file is part of acsgw
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.
// Licensed under the MIT License.
// See COPYING file in the project root for full license information.
//

/* -------------------------------------------------------------------------- */

#pragma once

#include "ConfigParser.h"
#include "Tools.h"
#include "MpTunnel.h"

#include <cassert>
#include <iostream>
#include <memory>
#include <stdlib.h>
#include <string.h>
#include <string>

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
type          ="udp"        # Tunnelling protocol
local_address ="10.0.0.1"   # Logical address of tunnel ingress
remote_address="10.0.0.2"   # Logical address of tunnel egress
port          = 28774

[tunnel2]
bearers        ="bearer1, bearer2" # Multiple baerers tunnel
type           ="gre"              
local_address  ="10.0.0.3"
remote_address ="10.0.0.4"
multipath      ="mirroring" # Defines the algo used in case of multiple paths*/


class TunnelBuilder
{
public:
   using Handle=std::shared_ptr<TunnelBuilder>;

   struct Bearer
   {
      std::string localAddress;
      std::string remoteAddress;
      TunnelProtocol tunnelProtocol { TunnelProtocol::Gre };
      int port = 28774; // Server port TCP/UDP transport
   };

   struct Tunnel
   {
      std::list<Bearer> bearers;
      std::string type = "gre";
      std::string localAddress;
      std::string remoteAddress;
      int port = 28774; // Server port TCP/UDP transport
   };

   using LookupTbl = std::map<std::string, Tunnel>;
   using LookupTblHandler = std::shared_ptr<LookupTbl>;

   TunnelBuilder() = delete;

   TunnelBuilder(Config &cfg);

   bool buildTunnels();

   LookupTblHandler getTunnelsTbl() const
   {
      return _tunnels;
   }

private:
   LookupTblHandler _tunnels;
   MpTunnelMgr _mpTunnelMgr;
   std::shared_ptr<VirtualIfMgr> _vifmgr;

   bool createTunnel(const std::string &ifname, const Tunnel &tunnel);
   static LookupTblHandler parseCfg(const Config &cfg);
};
