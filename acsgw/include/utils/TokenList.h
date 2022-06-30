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

#include "Exception.h"
#include "Token.h"

#include <deque>
#include <functional>
#include <list>
#include <ostream>
#include <set>


/* -------------------------------------------------------------------------- */

//! Defines the structure of an object that represents a collection of tokes
class TokenList {
private:
    std::deque<Token> _data;

public:
    using data_t = std::deque<Token>;
    static const size_t npos = size_t(-1);
    using tkp_t = std::pair<std::string, TokenClass>;
    using btfunc_t = std::function<bool(const Token&)>;

    // ctors & operators=

    TokenList() = default;
    TokenList(const TokenList&) = default;
    TokenList& operator=(const TokenList&) = default;

    TokenList(TokenList&& other) noexcept : 
        _data(std::move(other._data)) 
    {
    }

    TokenList& operator=(TokenList&& other) noexcept {
        if (this != &other) {
            _data = std::move(other._data);
        }
        return *this;
    }

    //! Return a reference to standard internal data
    data_t& data() noexcept { 
        return _data; 
    }

    //! Return a constant reference to standard internal data
    const data_t& data() const noexcept { 
        return _data; 
    }

    //! Removes all elements from the token list
    void clear() noexcept { 
        data().clear(); 
    }

    //! Returns whether the token list is empty
    bool empty() const noexcept { 
        return data().empty(); 
    }

    //! Returns the number of elements in the token list
    size_t size() const noexcept { 
        return data().size(); 
    }
    
    //! Returns an iterator pointing to the first element in the
    //! token list
    data_t::iterator begin() noexcept { 
        return data().begin(); 
    }
    
    //! Returns an iterator referring to the past-the-end
    //! element in the token list
    data_t::iterator end() noexcept { 
        return data().end(); 
    }

    //! Returns a reverse iterator pointing to the last element in the
    //! token list
    data_t::reverse_iterator rbegin() noexcept { 
        return data().rbegin(); 
    }
    
    //! Returns a reverse iterator pointing to the theoretical element
    //! preceding the first element in the token list
    data_t::reverse_iterator rend() noexcept { 
        return data().rend(); 
    }
    
    //! Returns a const_iterator pointing to the first element in the
    //! token list
    data_t::const_iterator cbegin() const noexcept { 
        return data().cbegin(); 
    }
    
    //! Returns a const_iterator pointing to the past-the-end
    //! element in the token list
    data_t::const_iterator cend() const noexcept { 
        return data().cend(); 
    }

    // insertion/append methods


    //! Append a new token at the tail of the token list
    TokenList& operator+=(const Token& tkn) noexcept {
        _data.push_back(tkn);
        return *this;
    }


    //! Return the list of tokens formed by the two tokens {ltkn, rtkn}
    friend TokenList operator+(const Token& ltkn, const Token& rtkn) {
        TokenList ret;
        ret += ltkn;
        ret += rtkn;
        return ret;
    }


    //! Return a list of tokens which is result of concatenation of tknl and tkn
    friend TokenList operator+(const TokenList& tknl, const Token& tkn) {
        TokenList ret = tknl;
        ret += tkn;
        return ret;
    }


    //! Return a list of token which is the result of concatenation
    //! of two given lists
    friend TokenList operator+(
        const TokenList& ltknl, const TokenList& rtknl)
    {
        TokenList ret = ltknl;
        ret += rtknl;
        return ret;
    }


    //! Append to the list all the tokens of a given token list argument
    TokenList& operator+=(const TokenList& tknl) {
        for (const auto& e : tknl.data()) {
            data().push_back(e);
        }

        return *this;
    }
    
    //! prefix - delete first token
    TokenList& operator--();

    //! postfix - delete last token
    TokenList& operator--(int);
    
    //! Return an iterator pointing to the next element of list
    //! of tokens enclosed in the two tokens 'first' and 'second'
    //! In case such a list does not exist, returns an iterator
    //! referring to the past-the-end element of the list
    data_t::iterator skip_right(
        data_t::iterator search_from, const tkp_t& first, const tkp_t& second);
    
    //! Return an (reverse) iterator pointing to the next element of list
    //! of tokens enclosed in the two tokens 'first' and 'second'
    //! In case such a list does not exist, returns an iterator
    //! referring to the past-the-end element of the list
    data_t::reverse_iterator skip_left(data_t::reverse_iterator search_from,
        const tkp_t& first, const tkp_t& second);
    
    //! Converts a reverse iterator into equivalent forward iterator
    data_t::iterator reverse_to_forward(data_t::reverse_iterator ri) {
        return data_t::iterator(begin() + (rend() - ri));
    }
    
    //! Converts a forward iterator into equivalent reverse iterator
    data_t::reverse_iterator forward_to_reverse(data_t::iterator i) {
        return data_t::reverse_iterator(rbegin() + (end() - i));
    }

    //! Returns the index of a token pointed by a given iterator
    size_t iterator_to_pos(data_t::iterator i) { 
        return i - begin(); 
    }
    
    // find

    //! Searches the token for the first occurrence of the sequence specified
    //! by its arguments

    size_t find(std::function<bool(size_t)> test, size_t pos, size_t items);
    size_t find(const Token& t, size_t pos = 0, size_t items = 0);
    size_t find(
        const std::string& identifier, size_t pos = 0, size_t items = 0);
    size_t find(const TokenClass type, size_t pos = 0, size_t items = 0);

    size_t find(const tkp_t& tkp, size_t pos = 0, size_t items = 0) {
        Token t(tkp.first, tkp.second, 0, nullptr);
        return find(t, pos, items);
    }

    // sublist

    //! Return a sub-list of tokens maching with search-criteria
    //! specified by its arguments

    TokenList sublist(size_t pos, size_t items);
    
    TokenList sublist(btfunc_t test_begin, btfunc_t test_end,
        size_t& search_from, bool b_erase);
    
    TokenList sublist(const Token& first, const Token& second,
        size_t search_from = 0, bool b_erase = false);
    
    TokenList sublist(const tkp_t& first, const tkp_t& second,
        size_t search_from = 0, bool b_erase = false);
    
    TokenList sublist(const std::string& first, const std::string& second,
        size_t search_from = 0, bool b_erase = false)
    {
        return sublist(
            [&](const Token& t) { return t.identifier() == first; },
            [&](const Token& t) { return t.identifier() == second; },
            search_from, b_erase);
    }


    TokenList sublist(const TokenClass first, const TokenClass second,
        size_t search_from, bool b_erase)
    {
        return sublist([&](const Token& t) { return t.type() == first; },
            [&](const Token& t) { return t.type() == second; }, search_from,
            b_erase);
    }


    // replace sublist

    //! Return the list of tokens replacing a sub-list matching the
    //! search-criteria specified by its arguments

    TokenList replace_sublist(btfunc_t test_begin, btfunc_t test_end,
        size_t& search_from, const TokenList& replist);
    
    TokenList replace_sublist(
        size_t begin_pos, size_t end_pos, const TokenList& replist);
    
    TokenList replace_sublist(const tkp_t& first, const tkp_t& second,
        size_t search_from, const TokenList& replist);


    // parameters

    //! Return a list of tokens which is enclosed in begin-token and
    //! end-token and separated by a separator token determinated by
    //! its test_<function> parameters
    std::list<TokenList> get_parameters(btfunc_t test_begin,
        btfunc_t test_end, btfunc_t test_separator, size_t search_from);

    //! Return a list of tokens which is enclosed in begin-token and
    //! end-token and separated by a separator token determinated by
    //! its parameters
    std::list<TokenList> get_parameters(
        const tkp_t& tbegin, const tkp_t& tend, const tkp_t& tseparator);


    // operator[]

    //! Returns a reference to the element at position idx
    //! in the token list
    Token operator[](size_t idx) const;
    Token& operator[](size_t idx);

private:
    void _chekpos(size_t pos, size_t items);
};


/* -------------------------------------------------------------------------- */

std::ostream& operator<<(std::ostream& os, const TokenList& tl);

