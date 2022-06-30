//  
// This file is part of acsgw
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.  
// Licensed under the MIT License. 
// See COPYING file in the project root for full license information.
//

/* -------------------------------------------------------------------------- */

#include "TokenList.h"
#include "SyntaxError.h"
#include "Exception.h"

#include <cassert>

/* -------------------------------------------------------------------------- */

// prefix
TokenList& TokenList::operator--()
{
    assert(!empty());

    data().erase(begin());
    return *this;
}


/* -------------------------------------------------------------------------- */

// postfix
TokenList& TokenList::operator--(int)
{
    assert(!empty());

    data().erase(end() - 1);
    return *this;
}


/* -------------------------------------------------------------------------- */

TokenList TokenList::sublist(size_t pos, size_t items)
{
    assert(!((pos + items) > size()));

    TokenList ret;

    for (size_t i = pos; i < (items + pos); ++i) {
        ret += (*this)[i];
    }

    return ret;
}


/* -------------------------------------------------------------------------- */

size_t TokenList::find(
    std::function<bool(size_t)> test, size_t pos, size_t items)
{
    if (!items)
        items = size();

    assert(!((pos + items) > size()));

    for (size_t i = pos; i < (pos + items); ++i) {
        if (test(i)) {
            return i;
        }
    }

    return npos;
}


/* -------------------------------------------------------------------------- */

size_t TokenList::find(const Token& t, size_t pos, size_t items)
{
    auto test = [&](size_t i) {
        return (*this)[i].identifier() == t.identifier()
            && (*this)[i].type() == t.type();
    };

    return find(test, pos, items);
}


/* -------------------------------------------------------------------------- */

size_t TokenList::find(
    const std::string& identifier, size_t pos, size_t items)
{
    auto test = [&](size_t i) { return (*this)[i].identifier() == identifier; };

    return find(test, pos, items);
}


/* -------------------------------------------------------------------------- */

size_t TokenList::find(const TokenClass type, size_t pos, size_t items)
{
    auto test = [&](size_t i) { return (*this)[i].type() == type; };

    return find(test, pos, items);
}


/* -------------------------------------------------------------------------- */

std::list<TokenList> TokenList::get_parameters(btfunc_t test_begin,
    btfunc_t test_end, btfunc_t test_separator, size_t search_from)
{
    assert(search_from < size());

    TokenList ret;
    int level = 0;
    size_t end_pos = 0;
    size_t begin_pos = 0;
    std::list<TokenList> ret_list;

    for (size_t i = search_from; i < size(); ++i) {
        const Token& t = (*this)[i];

        if (test_begin(t)) {
            if (level == 0)
                begin_pos = i;

            ++level;
        } 
        else if (test_end(t)) {
            --level;

            if (level < 1) {
                ret_list.push_back(ret);
                break;
            }
        }

        if (level == 1 && test_separator(t)) {
            ret_list.push_back(ret);
            ret.clear();

            if (i == end_pos)
                break;
        } 
        else if (level > 0) {
            if (i != begin_pos)
                ret += t;
        }
    }

    if (level > 0) {
        const Token& t = (*this)[end_pos];
        syntax_error(t.expression(), t.position(), "Missing token, error");
    }

    return ret_list;
}


/* -------------------------------------------------------------------------- */

std::list<TokenList> TokenList::get_parameters(
    const tkp_t& tbegin, const tkp_t& tend, const tkp_t& tseparator)
{
    auto fb = [&](const Token& t) {
        return t.identifier() == tbegin.first && t.type() == tbegin.second;
    };

    auto fe = [&](const Token& t) {
        return t.identifier() == tend.first && t.type() == tend.second;
    };

    auto fs = [&](const Token& t) {
        return t.identifier() == tseparator.first
            && t.type() == tseparator.second;
    };

    return get_parameters(fb, fe, fs, 0);
}


/* -------------------------------------------------------------------------- */

TokenList TokenList::sublist(
    std::function<bool(const Token&)> test_begin,
    std::function<bool(const Token&)> test_end, size_t& search_from,
    bool b_erase)
{
    assert(search_from < size());

    TokenList ret;
    int level = 0;
    size_t end_pos = 0;
    size_t begin_pos = 0;

    for (size_t i = search_from; i < size(); ++i) {
        const Token& t = (*this)[i];

        if (test_begin(t)) {
            if (level == 0)
                begin_pos = i;

            ++level;
        } 
        else if (test_end(t)) {
            --level;

            if (level < 1) {
                end_pos = i;
                ret += t;
                break;
            }
        }

        if (level > 0)
            ret += t;
    }

    if (level > 0) {
        const Token& t = (*this)[end_pos];

        syntax_error(t.expression(), t.position(), "Missing token, error");
    }

    if (b_erase) {
        data().erase(begin() + begin_pos, begin() + end_pos);
    }

    search_from = begin_pos;

    return ret;
}


/* -------------------------------------------------------------------------- */

TokenList TokenList::sublist(const Token& first, const Token& second,
    size_t search_from, bool b_erase)
{

    auto test_begin = [&](const Token& t) {
        return t.identifier() == first.identifier() && t.type() == first.type();
    };

    auto test_end = [&](const Token& t) {
        return t.identifier() == second.identifier()
            && t.type() == second.type();
    };

    return sublist(test_begin, test_end, search_from, b_erase);
}


//-------------------------------------------------------------------------

TokenList TokenList::sublist(
    const tkp_t& first, const tkp_t& second, size_t search_from, bool b_erase)
{

    auto test_begin = [&](const Token& t) {
        return t.identifier() == first.first && t.type() == first.second;
    };

    auto test_end = [&](const Token& t) {
        return t.identifier() == second.first && t.type() == second.second;
    };

    return sublist(test_begin, test_end, search_from, b_erase);
}


/* -------------------------------------------------------------------------- */

TokenList::data_t::iterator TokenList::skip_right(
    data_t::iterator search_from, const tkp_t& first, const tkp_t& second)
{
    assert(search_from != end());

    int level = 0;

    for (; search_from != end(); ++search_from) {
        const Token& t = *search_from;

        if (t.identifier() == first.first && t.type() == first.second) {
            ++level;
        }

        if (t.identifier() == second.first && t.type() == second.second) {
            --level;

            if (level < 1) {
                ++search_from;
                break;
            }
        }
    }

    if (level > 0) {
        const Token& t = *(end() - 1);

        syntax_error(t.expression(), t.position(), "Missing token, error");
    }

    return search_from;
}


/* -------------------------------------------------------------------------- */

TokenList::data_t::reverse_iterator TokenList::skip_left(
    data_t::reverse_iterator search_from, const tkp_t& first,
    const tkp_t& second)
{
    assert(search_from != rend());

    int level = 0;

    for (; search_from != rend(); ++search_from) {
        const Token& t = *search_from;

        if (t.identifier() == second.first && t.type() == second.second) {
            ++level;
        }

        if (t.identifier() == first.first && t.type() == first.second) {
            --level;

            if (level < 1) {
                ++search_from;
                break;
            }
        }
    }

    if (level > 0) {
        const Token& t = *(rend() - 1);
        syntax_error(t.expression(), t.position(), "Missing token, error");
    }

    return search_from;
}


/* -------------------------------------------------------------------------- */

TokenList TokenList::replace_sublist(
    std::function<bool(const Token&)> test_begin,
    std::function<bool(const Token&)> test_end, size_t& search_from,
    const TokenList& replist)
{
    assert(search_from < size());

    TokenList sub_list = sublist(test_begin, test_end, search_from, false);
    TokenList head_list = sublist(0, search_from);
    TokenList tail_list = sublist(
        search_from + sub_list.size(), size() - search_from - sub_list.size());

    return head_list + replist + tail_list;
}


/* -------------------------------------------------------------------------- */

TokenList TokenList::replace_sublist(
    size_t begin_pos, size_t end_pos, const TokenList& replist)
{
    assert(!(end_pos < begin_pos || end_pos >= size()));

    TokenList sub_list = sublist(begin_pos, end_pos - begin_pos + 1);
    TokenList head_list = sublist(0, begin_pos);
    TokenList tail_list = sublist(end_pos + 1, size() - end_pos - 1);

    return head_list + replist + tail_list;
}


/* -------------------------------------------------------------------------- */

TokenList TokenList::replace_sublist(const tkp_t& first,
    const tkp_t& second, size_t search_from, const TokenList& replist)
{
    auto test_begin = [&](const Token& t) {
        return t.identifier() == first.first && t.type() == first.second;
    };

    auto test_end = [&](const Token& t) {
        return t.identifier() == second.first && t.type() == second.second;
    };

    return replace_sublist(test_begin, test_end, search_from, replist);
}


/* -------------------------------------------------------------------------- */

Token TokenList::operator[](size_t idx) const
{
    assert(!empty());

    if (idx > size()) {
        auto i = (cend() - 1);
        syntax_error(i->expression(), i->position());
    }

    return data().operator[](idx);
}


/* -------------------------------------------------------------------------- */

Token& TokenList::operator[](size_t idx)
{
    assert(!empty());

    if (idx > size()) {
        auto i = (cend() - 1);
        syntax_error(i->expression(), i->position());
    }

    return data().operator[](idx);
}


/* -------------------------------------------------------------------------- */

void TokenList::_chekpos(size_t pos, size_t items)
{
    syntax_error_if((pos + items) > size(), "Internal error 13");
}


/* -------------------------------------------------------------------------- */

std::ostream& operator<<(std::ostream& os, const TokenList& tl)
{
    for (const auto& e : tl.data()) {
        if (e.identifier().empty()) {
            os << (e.type() == TokenClass::SUBEXP_BEGIN ? "{" : "}");
        }
        else {
            os << e.identifier();
        }
    }

    return os;
}
