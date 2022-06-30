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

#include "IpAddress.h"
#include <string>

/* -------------------------------------------------------------------------- */

class RouteMgr
{
public:
   bool add(const IpAddress &ip, const IpAddress &via, const std::string& mask="/32");
   bool add(const IpAddress &ip, const std::string &dev, const std::string& mask="/32");
   bool del(const IpAddress &ip, const std::string& mask="/32");
};
