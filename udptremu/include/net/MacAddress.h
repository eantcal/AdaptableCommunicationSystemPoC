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

#include <string>
#include <stdio.h>
#include <memory.h>
#include <array>

/* -------------------------------------------------------------------------- */

class MacAddress
{
public:
   enum
   {
      MAC_LEN = 6
   };

private:
   char _macaddr[MAC_LEN] = {0};

   char _getdigitval(const char c) const noexcept
   {
      return c >= '0' && c <= '9' ? c - '0' : c >= 'A' && c <= 'F' ? c - 'A' + 0xA
                                          : c >= 'a' && c <= 'f'   ? c - 'a' + 0xA
                                                                   : 0;
   }

   char _getbs2(const char *str) const noexcept
   {
      return (_getdigitval(str[0]) << 4) | _getdigitval(str[1]);
   }

   std::string _getpstr() const noexcept;

public:
   MacAddress() = default;

   MacAddress(const char mac[MAC_LEN]) noexcept
   {
      memcpy(&_macaddr[0], &mac[0], MAC_LEN);
   }

   MacAddress(const MacAddress &mac_obj) noexcept
   {
      memcpy(&_macaddr[0], &mac_obj._macaddr[0], MAC_LEN);
   }

   void assignTo(char dst_mac[MAC_LEN]) const noexcept
   {
      memcpy(&dst_mac[0], &_macaddr[0], MAC_LEN);
   }

   bool operator==(const MacAddress &mac) const noexcept
   {
      return memcmp(&_macaddr[0], &mac._macaddr[0], MAC_LEN) == 0;
   }

   explicit operator std::string() const noexcept
   {
      return _getpstr();
   }

   MacAddress(const std::string &mac) noexcept;
   MacAddress &operator=(const MacAddress &mac_obj) noexcept;
   char operator[](size_t i) const noexcept;
   char &operator[](size_t i) noexcept;
};
