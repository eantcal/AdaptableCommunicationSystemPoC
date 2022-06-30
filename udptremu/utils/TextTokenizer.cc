//
// This file is part of acsgw
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.
// Licensed under the MIT License.
// See COPYING file in the project root for full license information.
//

/* -------------------------------------------------------------------------- */

#include "TextTokenizer.h"

#include <algorithm>

/* -------------------------------------------------------------------------- */

bool WordScanner::accept(char c)
{
   // For each word in the list...
   for (const auto &keyword : _plist)
   {
      // Verify if the symbol belongs to the key-word
      if (keyword.find(c) != std::string::npos)
      {
         // Verify if <building-word>+<symbol> is sub-string of
         // key-word

         std::string tentative = _data;
         tentative.push_back(c);
         auto ldata = tentative.size();

         if (keyword.size() >= ldata && keyword.substr(0, ldata) == tentative)
         {
            // If tentative matches with key-word, save it and
            // terminates
            _data = tentative;
            keyword_matches = _data == keyword;

            return true;
         }
      }
   }

   return false;
}

/* -------------------------------------------------------------------------- */

bool SymbolScanner::accept(char c)
{
   if (_data.empty() && _plist.find(c) != _plist.cend())
   {
      _data = c;
      return true;
   }

   return false;
}

/* -------------------------------------------------------------------------- */

bool StringScanner::accept(char c)
{
   // escape can be accepted only if it is within the quotes
   if (_escape_prefix && !_escape_found && c == _escape_prefix)
   {

      if (_begin_found != _begin_quote || !_end_found.empty())
      {
         return false;
      }

      _escape_found = c;
      return true;
   }

   if (_begin_found != _begin_quote)
   {

      if (_bindex < _begin_quote.size() && c == _begin_quote[_bindex])
      {
         _begin_found += c;
         ++_bindex;
         return true;
      }

      return false;
   }

   else if (_end_found != _end_quote)
   {
      if (_escape_found)
      {
         switch (c)
         {
         case 'n':
            _data += '\n';
            break;

         case 'r':
            _data += '\r';
            break;

         case 'b':
            _data += '\b';
            break;

         case 't':
            _data += '\t';
            break;

         case 'a':
            _data += '\a';
            break;

         case 'u':
            _data += "\\u";
            break;

         default:
            _data += c;
         }

         _escape_found = 0;
         return true;
      }

      if (_eindex < _end_quote.size() && c == _end_quote[_eindex])
      {
         _end_found += c;
         ++_eindex;
      }
      else
      {
         _data += c;
      }

      return true;
   }

   return false;
}

/* -------------------------------------------------------------------------- */

void StringScanner::reset()
{
   _data.clear();
   _begin_found.clear();
   _end_found.clear();
   _bindex = 0;
   _eindex = 0;
   _escape_found = 0;
}

/* -------------------------------------------------------------------------- */

const std::string &StringScanner::data() const
{
   return _data;
}

/* -------------------------------------------------------------------------- */

TextTokenizer::TextTokenizer(
   const std::string &data, size_t pos,
   const std::string &blanks, const std::string &newlines,
   const std::string &operators, const std::set<std::string> &str_op,
   const char subexp_bsymb, // call/subexpr operators
   const char subexp_esymb, const std::string &string_bsymb,
   const std::string &string_esymb, const char string_escape,
   const std::set<std::string> &line_comment)
   : Tokenizer(data), 
      _pos(pos), _str_op(str_op), 
      _strtk(string_bsymb, string_esymb, string_escape), 
      _line_comment(line_comment)
{
   _subexp_begin_symb.push_back(subexp_bsymb);
   _subexp_end_symb.push_back(subexp_esymb);

   for (auto e : blanks)
   {
      _blank.register_pattern(e);
   }

   for (auto e : newlines)
   {
      _newl.register_pattern(e);
   }

   for (auto e : operators)
   {
      _op.register_pattern(e);
   }

   for (auto e : str_op)
   {
      _word_op.register_pattern(e);
   }

   if (!_line_comment.empty())
   {
      for (const auto &comment_word : line_comment)
      {
         if (comment_word.size() == 1)
         {
            _op.register_pattern(comment_word[0]);
         }
         else
         {
            _word_op.register_pattern(comment_word);
         }
      }
   }

   _op.register_pattern(subexp_bsymb);
   _op.register_pattern(subexp_esymb);
}

/* -------------------------------------------------------------------------- */

void TextTokenizer::getTokenList(TokenList &tl /*, bool strip_comment*/)
{
   tl.clear();

   if (eol())
   {
      return;
   }

   do
   {
      tl.data().push_back(next());
   } while (!eol());

   //if (strip_comment) {
   //    strip_comment_line(tl, _comment_line_set);
   //}
}

/* -------------------------------------------------------------------------- */

static inline bool identifier_char(char c)
{
   return ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') || c == '_');
}

/* -------------------------------------------------------------------------- */

Token TextTokenizer::next()
{
   // Fix . operator for real numbers

   auto pointer = tell();
   auto tkn = _next();

   if (!(tkn.type() == TokenClass::INTEGRAL || 
      (tkn.identifier() == "." && tkn.type() == TokenClass::OPERATOR)))
   {
      return tkn;
   }

   auto position = tkn.position();
   std::string id = tkn.identifier();

   if (tkn.type() == TokenClass::INTEGRAL)
   {
      tkn = _next();

      if (tkn.type() != TokenClass::OPERATOR || tkn.identifier() != ".")
      {
         setCPtr(pointer);
         return _next();
      }

      id += tkn.identifier();

      tkn = _next();

      if (tkn.type() != TokenClass::INTEGRAL)
      {
         setCPtr(pointer);
         return _next();
      }

      id += tkn.identifier();
   }
   else if (tkn.identifier() == "." && tkn.type() == TokenClass::OPERATOR)
   {
      tkn = _next();

      if (tkn.type() != TokenClass::INTEGRAL)
      {
         setCPtr(pointer);
         return _next();
      }

      id = "0." + tkn.identifier();
   }
   else
   {
      setCPtr(pointer);
      return _next();
   }

   tkn.set_identifier(id, Token::case_t::NOCHANGE);

   tkn.set_position(position);
   tkn.set_type(TokenClass::REAL);

   return tkn;
}

/* -------------------------------------------------------------------------- */

bool TextTokenizer::is_integer(const std::string &value)
{
   if (value.empty())
      return false;

   auto is_intexpr = [](char c) {
      return (c >= '0' && c <= '9');
   };

   const char first_char = value.c_str()[0];

   if (!is_intexpr(first_char) && first_char != '-')
      return false;

   if (value.size() == 1)
      return first_char != '-';

   for (size_t i = 1; i < value.size(); ++i)
   {
      const char c = value.c_str()[i];

      if (!is_intexpr(c))
         return false;
   }

   return true;
}

/* -------------------------------------------------------------------------- */

bool TextTokenizer::is_real(const std::string &value)
{
   if (value.empty())
   {
      return false;
   }

   auto is_intexpr = [](char c) {
      return (c >= '0' && c <= '9');
   };

   const char first_char = value.c_str()[0];

   if (!is_intexpr(first_char) && first_char != '-' && first_char != '.')
   {
      return false;
   }

   if (value.size() == 1)
   {
      return first_char != '-' && first_char != '.';
   }

   char old_c = 0;
   int point_cnt = 0;
   int E_cnt = 0;

   for (size_t i = 0; i < value.size(); ++i)
   {
      const char c = value.c_str()[i];

      const bool isValid = (c == '-' && i == 0) || 
         is_intexpr(c) || (c == '.' && point_cnt++ < 1) || 
         (::toupper(c) == 'E' && E_cnt++ < 1 && is_intexpr(old_c)) || 
         ((c == '+' || c == '-') && (::toupper(old_c) == 'E'));

      if (!isValid)
      {
         return false;
      }

      old_c = c;
   }

   // we accept also expression like 1E (it is incomplete floating expression
   // parser will verify if next token match with +/- and an integer exponent)
   if (!is_intexpr(old_c) && ::toupper(old_c) != 'E')
   {
      return false;
   }

   return true;
}

/* -------------------------------------------------------------------------- */

Token TextTokenizer::_next()
{
   std::string other;

   auto token_class = [&](const std::string &tk) -> TokenClass {
      if (is_integer(tk))
         return TokenClass::INTEGRAL;
      else if (is_real(tk))
         return TokenClass::REAL;

      // Resolve operator like "mod", "div", ...
      std::string tklowercase = tk;

      std::transform(tklowercase.begin(), tklowercase.end(),
                     tklowercase.begin(), tolower);

      return _str_op.find(tklowercase) != _str_op.end() ? TokenClass::OPERATOR
                                                        : TokenClass::IDENTIFIER;
   };

   auto extract_comment = [&](Token &token, std::string &comment) {
      while (!eol())
      {
         if (const auto symbol = getSymbol(); _newl.accept(symbol))
            break;

         comment += getSymbol();
         seekNext();
      }

      token.set_identifier(comment, Token::case_t::NOCHANGE);
      token.set_type(TokenClass::LINE_COMMENT);
   };

   _strtk.reset();
   _word_op.reset();

   char last_symbol = 0;

   while (!eol())
   {
      Token token = makeToken(tell() + get_exp_pos(), data());
      const char symbol = getSymbol();

      _blank.reset();
      _newl.reset();
      _op.reset();

      // Detect strings...
      if (_strtk.accept(symbol) && other.empty())
      {
         seekNext();

         if (eol())
         {
            token.set_identifier(_strtk.data(), Token::case_t::NOCHANGE);

            token.set_type(_strtk.string_complete()
                               ? TokenClass::STRING_LITERAL
                               : TokenClass::UNDEFINED);
            return token;
         }

         continue;
      }

      // Detect "word" operators <=, >=, <>, ...
      // They must be analyzed before "1-character" operators
      // like <,>,=, ...
      else if (_word_op.accept(symbol) && (!identifier_char(last_symbol) && /*->NOTE1*/
                                           !identifier_char(symbol)))       /*->NOTE1*/
      {
         /*NOTE1*/
         // operators like 'or', 'and', etc... may be part of
         // an identifier such as 'for', 'mandatory', etc...
         // we have to recognize this case in order to
         // avoid to split this identifier in different
         // tokens, like f-'or' or m-'and'-t'or'y !

         auto set_point = tell();
         seekNext();
         char symbol = getSymbol();

         while (!eol() && _word_op.accept(symbol))
         {

            if (_word_op.keyword_matches)
            {
               if (other.empty())
               {
                  seekNext();

                  if (identifier_char(symbol) && identifier_char(getSymbol()))
                  {
                     /*->NOTE1*/
                     break;
                  }

                  token.set_identifier(
                      _word_op.data(), Token::case_t::LOWER);

                  // If we detect line comment prefix
                  // include left part of line into the comment
                  if (_line_comment.find(_word_op.data()) != _line_comment.end())
                  {
                     std::string comment = _word_op.data();
                     extract_comment(token, comment);
                     return token;
                  }

                  token.set_identifier(_word_op.data(), Token::case_t::LOWER);
                  token.set_type(TokenClass::OPERATOR);
                  return token;
               }
               else
               {
                  _word_op.reset();
                  setCPtr(set_point);
                  token.set_identifier(other, Token::case_t::LOWER);
                  token.set_type(token_class(other));
                  return token;
               }
            }

            seekNext();
            symbol = getSymbol();
         }

         _word_op.reset();
         setCPtr(set_point);
      }

      if (_strtk.string_complete())
      {
         token.set_identifier(_strtk.data(), Token::case_t::NOCHANGE);
         token.set_type(TokenClass::STRING_LITERAL);
         return token;
      }

      bool is_operator = _op.accept(symbol);

      if (is_operator)
      {
         char ssymb[2] = {symbol};

         if (_line_comment.find(ssymb) != _line_comment.end())
         {
            std::string comment;
            extract_comment(token, comment);
            return token;
         }
      }

      if (_blank.accept(symbol) || _newl.accept(symbol) || is_operator)
      {

         if (other.empty())
         {
            std::string s_symbol;
            s_symbol.push_back(symbol);
            token.set_identifier(s_symbol, Token::case_t::LOWER);

            if (!_blank.data().empty())
            {
               token.set_type(TokenClass::BLANK);
            }
            else if (!_newl.data().empty())
            {
               token.set_type(TokenClass::NEWLINE);
            }
            else
            {
               if (token.identifier() == subexp_begin_symbol())
               {
                  token.set_type(TokenClass::SUBEXP_BEGIN);
               }
               else if (token.identifier() == subexp_end_symbol())
               {
                  token.set_type(TokenClass::SUBEXP_END);
               }
               else
               {
                  token.set_type(TokenClass::OPERATOR);
               }
            }

            seekNext();
         }
         else
         {
            token.set_identifier(other, Token::case_t::LOWER);
            token.set_type(token_class(other));

            other.clear();
         }

         return token;
      }

      other += symbol;
      seekNext();

      if (eol() && !other.empty())
      {
         token.set_identifier(other, Token::case_t::LOWER);
         token.set_type(token_class(other));
         return token;
      }

      last_symbol = symbol;
   }

   return makeToken(tell() + get_exp_pos(), data()); // empty token
}
