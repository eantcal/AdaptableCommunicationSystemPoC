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

#include "SipHeader.h"
#include "TextTokenizer.h"
#include <set>

/* ------------------------------------------------------------------------- */

struct SipTnkzr : public TextTokenizer
{
   SipTnkzr(const std::string &data, size_t pos = 0)
       : TextTokenizer(data, pos,
                       " \t\r",                  // blanks
                       "\n",                     // lf
                       "*/^,\\=\";:<>?'",        // single-char ops
                       {"<=", ">=", "==", "!="}, // multichar-ops
                       '(', ')',                 // sub expr
                       "\"", "\"", '\\',         // string
                       {})
   {
   }
};

/* -------------------------------------------------------------------------- */

class SipHeaderParser
{
private:
   static void removeBlanks(TokenList &tl);

   static std::string parseAttr(TokenList &tl,
                                Token &token,
                                const std::list<std::string> &expected_id,
                                TokenClass expected_cl = TokenClass::IDENTIFIER,
                                bool last = false);

   SipHeader::Handle parseItem(TokenList &tl);

   SipHeader::Handle parseRegisterInvite(
       Token token, TokenList &tl);

   SipHeader::Handle parseCSeq(
       Token token, TokenList &tl);

   SipHeader::Handle parseVia(
       Token token, TokenList &tl);

   SipHeader::Handle parseAuth(
       Token token, TokenList &tl);

   SipHeader::Handle parseFromTo(
       Token token, TokenList &tl);

   SipHeader::Handle parseGenAttr(
       Token token, TokenList &tl);

   void parseKVList(
       TokenList &tl,
       SipHeader &element,
       bool remove_head = true,
       const std::list<std::string> &eof_tokens = {});

   SipHeader::Handle parseCommaSepList(
       Token token, TokenList &tl);

   SipHeader::Handle parseResponseHeader(
       Token token, TokenList &tl);


public:
   SipHeaderParser() = default;
   SipHeaderParser(const SipHeaderParser &) = default;
   SipHeaderParser &operator=(const SipHeaderParser &) = default;

   SipHeader::Handle parse(TokenList &tl);
   SipHeader::Handle parse(TextTokenizer &st);
};
