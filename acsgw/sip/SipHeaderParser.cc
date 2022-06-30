//
// This file is part of acsgw
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.
// Licensed under the MIT License.
// See COPYING file in the project root for full license information.
//

/* -------------------------------------------------------------------------- */

#include "SipHeader.h"
#include "SipHeaderParser.h"

#include "TokenList.h"

#include <set>
#include <sstream>
#include <iomanip>

/* -------------------------------------------------------------------------- */

void SipHeaderParser::removeBlanks(TokenList &tl)
{
   while (!tl.empty() && (tl.begin()->type() == TokenClass::BLANK || tl.begin()->type() == TokenClass::LINE_COMMENT || tl.begin()->type() == TokenClass::NEWLINE))
   {
      --tl;
   }

   while (!tl.empty() && (tl.rbegin()->type() == TokenClass::BLANK || tl.begin()->type() == TokenClass::LINE_COMMENT))
   {
      tl--;
   }
}

/* -------------------------------------------------------------------------- */

std::string SipHeaderParser::parseAttr(
    TokenList &tl,
    Token &token,
    const std::list<std::string> &expected_ids,
    TokenClass expected_cl,
    bool remove_token)
{
   --tl;
   removeBlanks(tl);
   syntax_error_if(tl.empty(), token.expression(), token.position());
   token = *tl.begin();

   if (!expected_ids.empty())
   {
      bool found = false;

      syntax_error_if(
          token.type() != expected_cl,
          token.expression(),
          token.position());

      for (const auto &expected_id : expected_ids)
      {
         if (token.identifier() == expected_id)
         {
            found = true;
            break;
         }
      }

      syntax_error_if(
          !found,
          token.expression(),
          token.position());
   }

   if (remove_token)
   {
      --tl;
      removeBlanks(tl);
   }

   return token.identifier();
};

/* -------------------------------------------------------------------------- */

SipHeader::Handle SipHeaderParser::parseRegisterInvite(
    Token token, TokenList &tl)
{
   //IDNT[register] BLNK[ ] IDNT[sip] OPER[:] IDNT[host/ip] BLNK[ ] IDNT[sip] OPER[/] REAL[2.0]

   SipHeader::Handle handle = std::make_shared<SipHeader>(token.identifier());

   handle->_ht = SipHeader::HeaderType::METHOD;

   parseAttr(tl, token, {"sip", "sips"});
   parseAttr(tl, token, {":"}, TokenClass::OPERATOR);
   handle->kvm().insert(std::make_pair("host", parseAttr(tl, token, {})));

   --tl;
   removeBlanks(tl);
   syntax_error_if(tl.empty(), token.expression(), token.position());
   token = *tl.begin();

   if (token.type() == TokenClass::OPERATOR && token.identifier() == ":")
   {
      handle->kvm().insert(std::make_pair("port", parseAttr(tl, token, {})));
      --tl;
      removeBlanks(tl);
      syntax_error_if(tl.empty(), token.expression(), token.position());
      token = *tl.begin();
   }

   if (token.type() == TokenClass::OPERATOR && token.identifier() == ";")
   {
      parseKVList(tl, *handle, false, {"sip", "sips"});
      if (!tl.empty())
      {
         parseAttr(tl, token, {"/"}, TokenClass::OPERATOR);
         parseAttr(tl, token, {"2.0"}, TokenClass::REAL);
      }
      return handle;
   }

   if (!(token.type() == TokenClass::IDENTIFIER &&
         (token.identifier() == "sip" || token.identifier() == "sips")))
   {
      parseAttr(tl, token, {"sip", "sips"});
   }

   parseAttr(tl, token, {"/"}, TokenClass::OPERATOR);
   parseAttr(tl, token, {"2.0"}, TokenClass::REAL);

   return handle;
}

/* -------------------------------------------------------------------------- */

SipHeader::Handle SipHeaderParser::parseCSeq(
    Token token, TokenList &tl)
{
   //IDNT[cseq] OPER[:] BLNK[ ] INTG[1] BLNK[ ] IDNT[register]

   SipHeader::Handle handle = std::make_shared<SipHeader>(token.identifier());

   parseAttr(tl, token, {":"}, TokenClass::OPERATOR);
   auto seq = parseAttr(tl, token, {}, TokenClass::INTEGRAL);

   handle->kvm().insert(std::make_pair(parseAttr(tl, token, {}), seq));

   return handle;
}

/* -------------------------------------------------------------------------- */

SipHeader::Handle SipHeaderParser::parseVia(
    Token token, TokenList &tl)
{
   //Via: SIP/2.0/UDP 10.10.1.13:5060; branch=z9hG4bK78946131-99e1-de11-8845-080027608325; rport
   SipHeader::Handle handle = std::make_shared<SipHeader>("via");

   parseAttr(tl, token, {":"}, TokenClass::OPERATOR);
   parseAttr(tl, token, {"sip", "sips"});
   parseAttr(tl, token, {"/"}, TokenClass::OPERATOR);
   parseAttr(tl, token, {"2.0"}, TokenClass::REAL);
   parseAttr(tl, token, {"/"}, TokenClass::OPERATOR);

   handle->kvm().insert(std::make_pair("protcol", parseAttr(tl, token, {})));
   handle->kvm().insert(std::make_pair("host", parseAttr(tl, token, {})));

   --tl;
   removeBlanks(tl);
   syntax_error_if(tl.empty(), token.expression(), token.position());
   token = *tl.begin();

   if (token.type() == TokenClass::OPERATOR && token.identifier() == ":")
   {
      --tl;
      removeBlanks(tl);
      syntax_error_if(tl.empty(), token.expression(), token.position());
      token = *tl.begin();
      syntax_error_if(token.type() != TokenClass::INTEGRAL, token.expression(), token.position());
      handle->kvm().insert(std::make_pair("port", token.identifier()));
   }

   parseKVList(tl, *handle);

   return handle;
}

/* -------------------------------------------------------------------------- */

SipHeader::Handle SipHeaderParser::parseAuth(
    Token token, TokenList &tl)
{
   //Authorization: Digest username="user",realm="TestIMS.com", ...

   SipHeader::Handle handle = std::make_shared<SipHeader>("authorization");

   std::string protocol, host, port;

   parseAttr(tl, token, {":"}, TokenClass::OPERATOR);
   parseAttr(tl, token, {"digest"});

   --tl;
   removeBlanks(tl);
   syntax_error_if(tl.empty(), token.expression(), token.position());

   while (!tl.empty())
   {
      token = *tl.begin();
      syntax_error_if(token.type() != TokenClass::IDENTIFIER,
                      token.expression(), token.position());

      std::string key, value;

      key = token.identifier();

      --tl;
      removeBlanks(tl);
      syntax_error_if(tl.empty(), token.expression(), token.position());

      token = *tl.begin();
      syntax_error_if(token.type() != TokenClass::OPERATOR ||
                          token.identifier() != "=",
                      token.expression(), token.position());

      --tl;
      removeBlanks(tl);
      syntax_error_if(tl.empty(), token.expression(), token.position());
      token = *tl.begin();

      value = token.identifier();

      --tl;
      removeBlanks(tl);

      if (!tl.empty())
      {
         token = *tl.begin();
         syntax_error_if(token.type() != TokenClass::OPERATOR ||
                             token.identifier() != ",",
                         token.expression(), token.position());
         --tl;
         removeBlanks(tl);
         syntax_error_if(tl.empty(), token.expression(), token.position());
      }

      handle->kvm().insert(std::make_pair(key, value));
   }

   return handle;
}

/* -------------------------------------------------------------------------- */

SipHeader::Handle SipHeaderParser::parseFromTo(
    Token token, TokenList &tl)
{
   //IDNT[from] OPER[:] BLNK[ ] OPER[<] IDNT[sip] OPER[:] IDNT[13@10.10.1.99] OPER[>] OPER[;]

   SipHeader::Handle handle = std::make_shared<SipHeader>(token.identifier());

   parseAttr(tl, token, {":"}, TokenClass::OPERATOR);

   --tl;
   removeBlanks(tl);
   syntax_error_if(tl.empty(), token.expression(), token.position());
   token = *tl.begin();

   if (token.type() != TokenClass::OPERATOR)
   {
      handle->kvm().insert(std::make_pair("description", token.identifier()));
      --tl;
      removeBlanks(tl);
      syntax_error_if(tl.empty(), token.expression(), token.position());
      token = *tl.begin();
   }

   syntax_error_if(
       token.type() != TokenClass::OPERATOR || token.identifier() != "<",
       token.expression(),
       token.position());

   auto sip = parseAttr(tl, token, {"sip", "sips"}, TokenClass::IDENTIFIER, false);
   parseAttr(tl, token, {":"}, TokenClass::OPERATOR, true);

   syntax_error_if(tl.empty(), token.expression(), token.position());
   token = *tl.begin();

   handle->kvm().insert(std::make_pair(sip, token.identifier()));

   --tl;
   removeBlanks(tl);
   syntax_error_if(tl.empty(), token.expression(), token.position());
   token = *tl.begin();

   if (token.type() == TokenClass::OPERATOR && token.identifier() == ":")
   {
      --tl;
      removeBlanks(tl);
      syntax_error_if(tl.empty(), token.expression(), token.position());
      token = *tl.begin();

      handle->kvm().insert(std::make_pair("port", token.identifier()));

      --tl;
      removeBlanks(tl);
      syntax_error_if(tl.empty(), token.expression(), token.position());
      token = *tl.begin();
   }

   if (token.type() == TokenClass::OPERATOR && token.identifier() == ";")
   {
      parseKVList(tl, *handle, false, {">"});
      if (!tl.empty())
      {
         removeBlanks(tl);
         token = *tl.begin();
      }
   }

   syntax_error_if(
       token.type() != TokenClass::OPERATOR ||
           token.identifier() != ">",
       token.expression(), token.position());

   parseKVList(tl, *handle);

   return handle;
}

/* -------------------------------------------------------------------------- */

SipHeader::Handle SipHeaderParser::parseGenAttr(
    Token token, TokenList &tl)
{
   //IDNT[attribute] OPER[:] IDNT[anytoken]

   SipHeader::Handle handle = std::make_shared<SipHeader>(token.identifier());

   parseAttr(tl, token, {":"}, TokenClass::OPERATOR, true);

   std::string value;
   while (!tl.empty())
   {
      auto &token = *tl.begin();
      if (token.type() != TokenClass::NEWLINE &&
          (token.type() != TokenClass::BLANK ||
           token.identifier() != "\r"))
      {
         value += token.original_value();
      }
      --tl;
   }

   handle->kvm().insert(std::make_pair(value, ""));

   return handle;
}

/* -------------------------------------------------------------------------- */

void SipHeaderParser::parseKVList(
    TokenList &tl,
    SipHeader &element,
    bool remove_head,
    const std::list<std::string> &eof_tokens)
{
   if (remove_head)
   {
      --tl;
      removeBlanks(tl);
   }

   while (!tl.empty())
   {
      auto token = *tl.begin();

      if (!eof_tokens.empty())
      {
         bool break_while = false;
         for (const auto &eof_token : eof_tokens)
         {
            if (!eof_token.empty() && token.identifier() == eof_token)
               break_while = true;
            break;
         }
         if (break_while)
            break;
      }

      syntax_error_if(token.type() != TokenClass::OPERATOR ||
                          token.identifier() != ";",
                      token.expression(), token.position());

      std::string key, value;

      --tl;
      removeBlanks(tl);
      if (!tl.empty())
      {
         token = *tl.begin();
         key = token.identifier();
         --tl;
         removeBlanks(tl);
         if (!tl.empty())
         {
            token = *tl.begin();
            syntax_error_if(token.type() != TokenClass::OPERATOR ||
                                token.identifier() != "=",
                            token.expression(), token.position());
            --tl;
            removeBlanks(tl);
            syntax_error_if(tl.empty(), token.expression(), token.position());
            token = *tl.begin();
            value = token.identifier();
            --tl;
            removeBlanks(tl);
         }
      }

      element.kvm().insert(std::make_pair(key, value));
   }
}

/* -------------------------------------------------------------------------- */

SipHeader::Handle SipHeaderParser::parseCommaSepList(
    Token token, TokenList &tl)
{
   SipHeader::Handle handle = std::make_shared<SipHeader>(token.identifier());

   parseAttr(tl, token, {":"}, TokenClass::OPERATOR, true);

   int n = 0;

   while (!tl.empty())
   {
      auto token = *tl.begin();

      handle->kvm().insert(std::make_pair(token.identifier(), ""));

      --tl;
      removeBlanks(tl);

      if (!tl.empty())
      {
         token = *tl.begin();
         syntax_error_if(token.type() != TokenClass::OPERATOR ||
                             token.identifier() != ",",
                         token.expression(), token.position());
         --tl;
         removeBlanks(tl);
      }
   }

   return handle;
}

/* -------------------------------------------------------------------------- */

SipHeader::Handle SipHeaderParser::parseResponseHeader(
    Token token, TokenList &tl)
{
   //IDNT[cseq] OPER[:] BLNK[ ] INTG[1] BLNK[ ] IDNT[register]

   SipHeader::Handle handle = std::make_shared<SipHeader>(token.identifier());

   handle->_ht = SipHeader::HeaderType::RESPONSE;

   parseAttr(tl, token, {"/"}, TokenClass::OPERATOR);
   parseAttr(tl, token, {"2.0"}, TokenClass::REAL);
   auto code = parseAttr(tl, token, {}, TokenClass::INTEGRAL, true);

   try
   {
      handle->_responseCode = std::stoi(code);
   }
   catch (...)
   {
   }

   std::string description;
   while (!tl.empty())
   {
      description += tl.begin()->original_value();
      --tl;
   }

   handle->kvm().insert(std::make_pair(code, description));

   return handle;
}

/* -------------------------------------------------------------------------- */

SipHeader::Handle SipHeaderParser::parseItem(TokenList &tl)
{
   removeBlanks(tl);

   if (tl.empty())
   {
      return SipHeader::Handle(std::make_shared<SipHeader>());
   }

   Token token(*tl.begin());

   if (token.type() == TokenClass::NEWLINE)
   {
      return std::make_shared<SipHeader>();
   }

   const std::string &identifier = token.identifier();

   if (identifier == "register" || identifier == "invite")
   {
      return parseRegisterInvite(token, tl);
   }

   else if (identifier == "cseq")
   {
      return parseCSeq(token, tl);
   }

   else if (identifier == "via")
   {
      return parseVia(token, tl);
   }

   else if (identifier == "authorization" || identifier == "www-authenticate")
   {
      return parseAuth(token, tl);
   }

   else if (identifier == "from" || 
            identifier == "to" || 
            identifier == "contact"
            )
   {
      return parseFromTo(token, tl);
   }

   else if (identifier == "allow" || identifier == "supported")
   {
      return parseCommaSepList(token, tl);
   }

   else if (identifier == "sip" || identifier == "sips")
   {
      return parseResponseHeader(token, tl);
   }
   else
   {
      return parseGenAttr(token, tl);
   }

   throw Exception("Syntax error");
}

/* -------------------------------------------------------------------------- */

SipHeader::Handle SipHeaderParser::parse(TokenList &tl)
{
   std::stringstream log;

   //for (auto t : tl) {
   //   log << Token::getDescription(t.type()) << "[" << t.identifier()
   //      << "] ";
   //}

   //printf("%s\n", log.str().c_str());

   removeBlanks(tl);

   if (!tl.empty())
   {
      Token token(*tl.begin());
      return parseItem(tl);
   }

   return std::make_shared<SipHeader>();
}

/* -------------------------------------------------------------------------- */

SipHeader::Handle SipHeaderParser::parse(TextTokenizer &st)
{
   TokenList tl;
   st.getTokenList(tl);
   return parse(tl);
}

/* -------------------------------------------------------------------------- */
