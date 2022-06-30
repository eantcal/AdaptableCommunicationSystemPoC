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
#include "TextTokenizer.h"
#include <set>


/* ------------------------------------------------------------------------- */

struct ConfigTnkzr : public TextTokenizer
{
   ConfigTnkzr(const std::string& data, size_t pos = 0)
      : TextTokenizer(data, pos,
         " \t\r", // blanks
         "\n",    // lf
         "=[]", // single-char ops
         { }, // multichar-ops
         '(', ')',    // sub expr
         "\"", "\"", '\\', // string
         {"#"}
      )
   {
   }
};


/* -------------------------------------------------------------------------- */

class ConfigParser {
private:
   static void removeBlanks(TokenList& tl, bool include_newline = false);
   void buildConfig(TokenList& tl, Config& cfg);

public:
   ConfigParser() = default;
   ConfigParser(const ConfigParser&) = default;
   ConfigParser& operator=(const ConfigParser&) = default;

   Config::Handle parse(TextTokenizer& st);
   Config::Handle parse(TokenList& tl);
};


/* -------------------------------------------------------------------------- */

