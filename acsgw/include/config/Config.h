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
#include "TextTokenizer.h"

#include <cassert>
#include <list>
#include <memory>
#include <unordered_map>
#include <ostream>
#include <list>

/* -------------------------------------------------------------------------- */

/* -- config example --

# this is a comment
# Default (global) namespace is [] (empty string)

[Namespace1]
a=1
b=2
c="abc"

[NameSpace2]
x=$Namespace1.a
y=1
k=$x
x=$_dev.HOSTNAME

*/

class Config
{
public:
   Config() = default;
   Config(const Config &) = delete;
   Config &operator=(const Config &) = delete;

   using Handle = std::shared_ptr<Config>;

   enum class Type
   {
      STRING,
      NUMBER,
      OTHERS
   };

   struct LineData
   {
      std::string namespaceId;
      std::string identifier;
      std::string value;
      Type valueType;
   };

   using Data = std::list<LineData>;

   using ConfigNamespacedParagraph = std::map<std::string, std::pair<std::string, Type>>;
   using ConfigData = std::map<std::string, ConfigNamespacedParagraph>;

   virtual ~Config(){};

   void describe(std::ostream &os);

   void selectNameSpace(const std::string &nameSpace = "" /*empty == default*/) const
   {
      _namespace = nameSpace;
   }

   void add(const std::string &name, const std::string &value, Type type);

   const ConfigData &data() const
   {
      return _data;
   }

   void compile();

   std::list<std::string> getAttrList(const std::string &name) const;

   std::string getAttr(const std::string &name) const;

private:
   mutable std::string _namespace = "global"; // default empty
   ConfigData _data;
   Data _rowData;

   int _lines = 0;
};
