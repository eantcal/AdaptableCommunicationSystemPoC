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
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstdint>
#include <sstream>
#include <ostream>

/* -------------------------------------------------------------------------- */

class IpAddress
{
private:
   using _ip_addr_t = uint32_t;

   typedef char _ip_addr_v_t[sizeof(_ip_addr_t)];

   union _ip_addr_u_t
   {
      _ip_addr_t _ip_addr;
      _ip_addr_v_t _ip_addr_v;
   } _ip_u;

   std::string _hostname;

public:
   explicit IpAddress(unsigned int ip_addr = 0) noexcept
   {
      _ip_u._ip_addr = ip_addr;
   }

   explicit IpAddress(const std::string &hostname) noexcept
   {
      hostent * record = gethostbyname(hostname.c_str());
      
      if (record) {
         const in_addr * address = (in_addr * )record->h_addr;
         _ip_u._ip_addr = ntohl(address->s_addr);
         _hostname = hostname;
      }
      else {
         in_addr_t ip_addr_value = inet_addr(hostname.c_str());
         _ip_u._ip_addr = ntohl(((in_addr *)&ip_addr_value)->s_addr);
      }
   }

   IpAddress(const IpAddress &ip_obj) noexcept
   {
      _ip_u._ip_addr = ip_obj._ip_u._ip_addr;
   }

   IpAddress &operator=(const IpAddress &ip_obj) noexcept
   {
      if (this != &ip_obj)
         _ip_u._ip_addr = ip_obj._ip_u._ip_addr;

      return *this;
   }

   bool operator==(const IpAddress &ip_obj) const noexcept
   {
      return _ip_u._ip_addr == ip_obj._ip_u._ip_addr;
   }

   bool operator<(const IpAddress &ip_obj) const noexcept
   {
      return _ip_u._ip_addr < ip_obj._ip_u._ip_addr;
   }

   char &operator[](size_t i) noexcept
   {
#ifndef BIGENDIAN
      return _ip_u._ip_addr_v[3 - i];
#else
      return _ip_u._ip_addr_v[i];
#endif
   }

   const std::string& getHostName() const noexcept {
      return _hostname;
   }

   std::string to_str() const noexcept
   {
      std::stringstream ss;
      ss << ((_ip_u._ip_addr >> 24) & 0xff) << "."
         << ((_ip_u._ip_addr >> 16) & 0xff) << "."
         << ((_ip_u._ip_addr >> 8) & 0xff) << "."
         << ((_ip_u._ip_addr >> 0) & 0xff);

      return ss.str();
   }

   friend std::ostream &operator<<(std::ostream &os, const IpAddress &ip)
   {
      os << ip.to_str();
      return os;
   }

   explicit operator std::string() const noexcept
   {
      return to_str();
   }

   explicit operator unsigned int() const noexcept
   {
      return to_n<unsigned int>();
   }

   explicit operator unsigned long() const noexcept
   {
      return to_n<unsigned long>();
   }

   explicit operator int() const noexcept
   {
      return to_n<int>();
   }

   explicit operator long() const noexcept
   {
      return to_n<long>();
   }

   template <class T>
   T to_n() const noexcept
   {
      return (T)(_ip_u._ip_addr);
   }

   unsigned int to_uint32() const noexcept
   {
      return to_n<uint32_t>();
   }

   int to_int() const noexcept
   {
      return to_n<int>();
   }

   unsigned long to_ulong() const noexcept
   {
      return to_n<unsigned long>();
   }

   long to_long() const noexcept
   {
      return to_n<long>();
   }
   
};
