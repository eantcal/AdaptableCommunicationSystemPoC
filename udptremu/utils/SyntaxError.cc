//  
// This file is part of acsgw
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.  
// Licensed under the MIT License. 
// See COPYING file in the project root for full license information.
//


/* -------------------------------------------------------------------------- */

#include "SyntaxError.h"
#include <cassert>
#include <string>

/* -------------------------------------------------------------------------- */

void syntax_error(const std::string& expr, size_t pos, const std::string& msg)
{
   std::string errDesc(msg.empty() ? "Syntax Error" : msg);
   
   std::string err;

   if (expr.size() > pos) {
      for (size_t i = 0; i < pos; ++i)
         err += expr[i];

      err += " <-- " + errDesc;
   }

   throw Exception(err);
}


/* -------------------------------------------------------------------------- */

void syntax_error_if(
   bool condition, const std::string& expr, size_t pos, const std::string& msg)
{
   if (condition) {
      syntax_error(expr, pos, msg);
   }
}


/* -------------------------------------------------------------------------- */

void syntax_error_if(bool condition, const std::string& msg)
{
   if (condition) {
      throw Exception(msg);
   }
}

