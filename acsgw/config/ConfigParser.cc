//  
// This file is part of acsgw
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.  
// Licensed under the MIT License. 
// See COPYING file in the project root for full license information.
//

/* -------------------------------------------------------------------------- */


#include "Config.h"
#include "ConfigParser.h"
#include "TokenList.h"

#include <set>
#include <sstream>
#include <iomanip>


/* -------------------------------------------------------------------------- */

void ConfigParser::removeBlanks(TokenList& tl, bool include_newline)
{
   while (!tl.empty() && (tl.begin()->type() == TokenClass::BLANK
      || tl.begin()->type() == TokenClass::LINE_COMMENT
      || (include_newline && tl.begin()->type() == TokenClass::NEWLINE)))
   {
      --tl;
   }

   
   while (!tl.empty() && (tl.rbegin()->type() == TokenClass::BLANK
      || tl.begin()->type() == TokenClass::LINE_COMMENT))
   {
      tl--;
   }
   
}


/* -------------------------------------------------------------------------- */

void ConfigParser::buildConfig(TokenList& tl, Config& config)
{
   //removeBlanks(tl);

   while (!tl.empty()) {
      auto token = *tl.begin();

      if (token.type() == TokenClass::NEWLINE || 
          token.type() == TokenClass::LINE_COMMENT || 
          token.type() == TokenClass::BLANK)
      {
         --tl;
         continue;
      }

      const bool parsing_namespace =
         token.type() == TokenClass::OPERATOR &&
         token.identifier() == "[";

      if (parsing_namespace) {
         --tl;
         removeBlanks(tl);
         token = *tl.begin();

         // [] means no namespace (global one)
         if (token.type() == TokenClass::OPERATOR && token.identifier() == "]") {
            config.selectNameSpace(); // default
            --tl;
            removeBlanks(tl, true);
            syntax_error_if(
               tl.empty(),
               token.expression(), token.position(), "Expected newline");
            continue;
         }
      }

      syntax_error_if(
         token.type() != TokenClass::IDENTIFIER,
         token.expression(), token.position(), "Expected an identifier");

      std::string name = token.identifier();

      std::string value;
      Config::Type type = Config::Type::OTHERS;

      --tl;
      removeBlanks(tl);

      if(!tl.empty(),
         token.expression(), token.position()) 
      {
         auto token = *tl.begin();

         if (parsing_namespace)
            syntax_error_if(token.type() != TokenClass::OPERATOR ||
               token.identifier() != "]", token.expression(), token.position(),
               "Expected operator ]");
         else 
            syntax_error_if(token.type() != TokenClass::OPERATOR ||
               token.identifier() != "=", token.expression(), token.position(),
               "Expected the assignement operator =");

         --tl;
         removeBlanks(tl);

         if (!parsing_namespace) {

            syntax_error_if(tl.empty(), token.expression(), token.position(),
               "Expected a value here");

            token = *tl.begin();

            syntax_error_if(token.type() == TokenClass::OPERATOR,
               token.expression(), token.position(),
               "The value should be a string or a number");

            value = token.original_value();

            if (token.type() == TokenClass::STRING_LITERAL) {
               type = Config::Type::STRING;
            }
            else if (token.type() == TokenClass::INTEGRAL || token.type() == TokenClass::REAL) {
               type = Config::Type::NUMBER;
            }

            --tl;
            removeBlanks(tl, true);
         }
         else {
            config.selectNameSpace(name);
            continue;
         }
      }

      config.add(name, value, type);
   }
}


/* -------------------------------------------------------------------------- */

Config::Handle ConfigParser::parse(TokenList& tl)
{
   removeBlanks(tl);

   if (!tl.empty()) {
      Config::Handle h = std::make_shared<Config>();
      assert(h);
      buildConfig(tl, *h);
      return h;
   }

   return nullptr;
}


/* -------------------------------------------------------------------------- */

Config::Handle ConfigParser::parse(TextTokenizer& st)
{
   TokenList tl;
   st.getTokenList(tl);
   return parse(tl);
}
