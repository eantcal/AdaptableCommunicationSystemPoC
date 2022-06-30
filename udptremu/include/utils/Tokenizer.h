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

#include "Token.h"
#include "TokenList.h"

#include <cassert>
#include <string>


/* -------------------------------------------------------------------------- */

/**
 * This is abstract tokenizer which provides a container view interface
 * of a series of tokens contained in a data text buffer
 */
class Tokenizer {
public:
    /** ctor
     * \param data: input buffer
     * \param pos: original position of input data
     */
    Tokenizer(const std::string& data)
        : _data(std::make_shared<std::string>(data))
        , _data_len(data.size())
    {
        assert(_data);
    }


    Tokenizer() = delete;
    Tokenizer(const Tokenizer&) = delete;
    Tokenizer& operator=(const Tokenizer&) = delete;

    //! dtor
    virtual ~Tokenizer() {}

    //! Get a token and advance to the next one (if any)
    virtual Token next() = 0;

    //! Get a token list
    virtual void getTokenList(TokenList& tl/*, bool strip_comment*/) = 0;


    //! Get buffer pointer (cptr)
    size_t tell() const noexcept { 
        return _cptr; 
    }


    //! Get size of the input buffer
    size_t size() const noexcept { 
        return _data_len; 
    }


    //! Return true if current position >= size()
    bool eol() const noexcept { 
        return tell() >= size(); 
    }


    //! Reset buffer pointer to zero
    void reset() noexcept { 
        _cptr = 0; 
    }

    //! Create a token object
    static Token makeToken(size_t pos, Token::DataPtr data_ptr) {
        return Token(pos, data_ptr);
    }


protected:
    //! Get current pointed character within the buffer
    char getSymbol() const noexcept { 
        return !eol() ? (*_data)[tell()] : 0; 
    }


    //! If not eof, move buffer pointer to next symbol
    void seekNext() noexcept {
        if (tell() < size()) {
            ++_cptr;
        }
    }


    //! Assign a new value to cptr
    void setCPtr(size_t cptr) noexcept { 
        _cptr = cptr; 
    }


    //! If not begin of buffer, move cptr to the previous symbol
    void seekPrev() noexcept {
        if (tell()) {
            --_cptr;
        }
    }


    //! Return a shared_ptr to internal data
    Token::DataPtr data() const noexcept { 
        return _data; 
    }


private:
    std::shared_ptr<std::string> _data;
    size_t _data_len = 0;
    size_t _cptr = 0;
};


