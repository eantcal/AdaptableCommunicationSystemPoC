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

#include "Tokenizer.h"

#include "Token.h"
#include "TokenList.h"

#include <deque>
#include <ostream>
#include <set>


/* -------------------------------------------------------------------------- */

class ChScanner {
public:
   virtual bool accept(char c) = 0;
   virtual const std::string& data() const = 0;
   virtual void reset() = 0;
};


/* -------------------------------------------------------------------------- */

template <class T> class AtomicWordScanner : public ChScanner {
protected:
   using pmset_t = std::set<T>;
   pmset_t _plist;
   std::string _data;

public:
   AtomicWordScanner() = default;
   AtomicWordScanner(const AtomicWordScanner&) = default;
   AtomicWordScanner& operator=(const AtomicWordScanner&) = default;

   void register_pattern(const T& pattern) {
      _plist.insert(pattern);
   }

   void remove_pattern(const T& pattern) {
      _plist.erase(pattern);
   }

   void reset() override {
      _data.clear();
   }

   void clear_patterns() {
      _plist.clear();
   }

   const std::string& data() const override {
      return _data;
   }
};


/* -------------------------------------------------------------------------- */

class WordScanner : public AtomicWordScanner<std::string> {
public:
   bool keyword_matches = false;

   WordScanner() = default;
   WordScanner(const WordScanner&) = default;
   WordScanner& operator=(const WordScanner&) = default;

   bool accept(char c) override;

   void reset() override {
      AtomicWordScanner<std::string>::reset();
      keyword_matches = false;
   }
};


/* -------------------------------------------------------------------------- */

class SymbolScanner : public AtomicWordScanner<char> {
public:
   SymbolScanner() = default;
   SymbolScanner(const SymbolScanner&) = default;
   SymbolScanner& operator=(const SymbolScanner&) = default;

   bool accept(char c) override;
};


/* -------------------------------------------------------------------------- */

class StringScanner : public ChScanner {
private:
   std::string _begin_quote, _end_quote;
   std::string _begin_found, _end_found;
   char _escape_prefix = 0;
   char _escape_found = 0;
   size_t _bindex = 0, _eindex = 0;
   std::string _data;

public:
   StringScanner(const std::string& begin_quote, const std::string& end_quote,
      char escape_prefix = 0)
      : _begin_quote(begin_quote)
      , _end_quote(end_quote)
      , _escape_prefix(escape_prefix)
   {
   }

   bool accept(char c) override;
   void reset() override;
   const std::string& data() const override;

   bool string_complete() const {
      return _end_found == _end_quote;
   }
};


/**
 * This class implements a tokenizer which provides a container view interface
 * of a series of tokens contained in a given expression string
 */
class TextTokenizer : public Tokenizer {
public:
    TextTokenizer() = delete;
    TextTokenizer(const TextTokenizer&) = delete;
    TextTokenizer& operator=(const TextTokenizer&) = delete;
    virtual ~TextTokenizer() {}

    TextTokenizer(const std::string& data, // expression string data
        size_t pos=0, // original position of expression
        // used only to generate syntax error info
        // Each token position marker is the result of the sum
        // between this value and the token position
        // in the expression
        const std::string& blanks=" \t", // Blanks symbols
        const std::string& newlines="\r\n", // New line symbols
        const std::string& operators=";", // Single char operators
        const std::set<std::string>& str_op={}, // Word operators
        const char subexp_bsymb='<', // Begin sub-expression symbol
        const char subexp_esymb='>', // End sub-expression symbol
        const std::string& string_bsymb="\"", // Begin string marker
        const std::string& string_esymb="\"", // End string marker
        const char string_escape='\\', // Escape string symbol
        const std::set<std::string>& line_comment={"#"}
        );


    //! Get a token and advance to the next one (if any)
    Token next() override;
    
    //! Split expression in token and copy them into a token list
    void getTokenList(TokenList& tl/*, bool strip_comment = true*/) override;


    using typed_token_id_t = std::pair<std::string, TokenClass>;
    using typed_token_set_t = std::set<typed_token_id_t>;

    //! Get begin sub-expression symbol
    std::string subexp_begin_symbol() const noexcept {
        return _subexp_begin_symb;
    }

    //! Get end sub-expression symbol
    std::string subexp_end_symbol() const noexcept { 
        return _subexp_end_symb; 
    }
    
    //! Return expression position in the source line
    size_t get_exp_pos() const noexcept { 
        return _pos; 
    }


   //Register new operators set
   void set_operators(const std::string& operators) {
      _op.clear_patterns();
      for (auto e : operators) {
         _op.register_pattern(e);
      }
   }


protected:
    //! Get a token and advance to the next one (if any)
    Token _next();

    size_t _pos = 0;
    std::string _subexp_begin_symb;
    std::string _subexp_end_symb;
    std::set<std::string> _str_op; // mod, div, ...
    SymbolScanner _blank, _op, _newl;
    WordScanner _word_op;
    StringScanner _strtk;

    std::set<std::string> _line_comment;

    static bool is_integer(const std::string& value);
    static bool is_real(const std::string& value);
};

