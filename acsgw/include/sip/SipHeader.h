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

#include "SyntaxError.h"
#include "Exception.h"

#include <cassert>
#include <list>
#include <memory>
#include <map>
#include <ostream>

/* -------------------------------------------------------------------------- */

class SipHeader
{
   friend class SipHeaderParser;

public:
   enum class HeaderType
   {
      METHOD,
      RESPONSE,
      OTHERS
   };

   SipHeader() = default;
   SipHeader(const SipHeader &) = default;
   SipHeader &operator=(const SipHeader &) = default;

   SipHeader(const std::string &name) noexcept : _name(name){};

   using key_value_map_t = std::map<std::string, std::string>;
   using Handle = std::shared_ptr<SipHeader>;

   virtual ~SipHeader();

   void describe(std::ostream &os)
   {
      os << "name: " << _name << "< ";
      for (const auto &[key, value] : _kvm)
      {
         os << "[" << key;

         if (!value.empty())
            os << " : " << value;
         
         os << "]";
      }
      os << ">";
   }

   HeaderType getType() const noexcept
   {
      return _ht;
   }

   int getResponseCode() const noexcept
   {
      return _responseCode;
   }

   const std::string &name() const
   {
      return _name;
   }

   const key_value_map_t &kvm() const
   {
      return _kvm;
   }

   key_value_map_t &kvm()
   {
      return _kvm;
   }

   SipHeader::Handle clone() 
   {
      return SipHeader::Handle(std::make_shared<SipHeader>(*this));
   }

private:
   HeaderType _ht = HeaderType::OTHERS;
   int _responseCode = 0;
   std::string _name;
   key_value_map_t _kvm;
};
