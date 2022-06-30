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

#include <algorithm>
#include <deque>
#include <memory>
#include <string>


/* -------------------------------------------------------------------------- */

//! Tonken class indentifier
enum class TokenClass {
    UNDEFINED,
    BLANK,
    NEWLINE,
    IDENTIFIER,
    INTEGRAL,
    REAL,
    OPERATOR,
    SUBEXP_BEGIN,
    SUBEXP_END,
    STRING_LITERAL,
    STRING_COMMENT,
    SUBSCR_BEGIN,
    SUBSCR_END,
    LINE_COMMENT
};


/* -------------------------------------------------------------------------- */

/**
 * This class holds a token data
 */
class Token {
    friend class Tokenizer;

public:
    enum class case_t { 
        LOWER, NOCHANGE 
    };

    using DataPtr = std::shared_ptr<std::string>;

    Token(const std::string& id, TokenClass t, size_t pos,
        DataPtr expr_ptr) noexcept;

    Token() = delete;

    Token(const Token& other) noexcept
        : _identifier(other._identifier)
        , _org_id(other._org_id)
        , _type(other._type)
        , _position(other._position)
        , _expression_ptr(other._expression_ptr)
    {
    }

    Token& operator=(const Token& other) noexcept {
        if (this != &other) {
            _identifier = other._identifier;
            _org_id = other._org_id;
            _type = other._type;
            _position = other._position;
            _expression_ptr = other._expression_ptr;
        }

        return *this;
    }

    void set_identifier(const std::string& id, case_t casemode);

    const std::string& identifier() const noexcept { 
        return _identifier; 
    }

    const std::string& original_value() const noexcept { 
        return _org_id; 
    }

    TokenClass type() const noexcept { 
        return _type; 
    }

    void set_type(TokenClass cl) noexcept { 
        _type = cl; 
    }

    size_t position() const noexcept { 
        return _position; 
    }

    void set_position(size_t pos) noexcept { 
        _position = pos; 
    }

    std::string expression() const noexcept { 
        return *_expression_ptr; 
    }

    DataPtr expression_ptr() const noexcept { 
        return _expression_ptr; 
    }

    Token(Token&& other) noexcept
        : _identifier(std::move(other._identifier))
        , _org_id(std::move(other._org_id))
        , _type(std::move(other._type))
        , _position(std::move(other._position))
        , _expression_ptr(std::move(other._expression_ptr))
    {
    }

    Token& operator=(Token&& other) noexcept {
        if (this != &other) {
            _identifier = std::move(other._identifier);
            _org_id = std::move(other._org_id);
            _type = std::move(other._type);
            _position = std::move(other._position);
            _expression_ptr = std::move(other._expression_ptr);
        }

        return *this;
    }

protected:
    Token(size_t pos, DataPtr expr_ptr) noexcept
        : _position(pos),
          _expression_ptr(expr_ptr)
    {
    }

private:
    std::string _identifier;
    std::string _org_id;
    TokenClass _type = TokenClass::UNDEFINED;
    size_t _position = 0;
    DataPtr _expression_ptr;

    void _transformLoCase() {
        std::transform(_identifier.begin(), _identifier.end(),
            _identifier.begin(), tolower);
    }

public:
    static std::string getDescription(TokenClass tc);
    friend std::ostream& operator<<(std::ostream& os, const Token& t);
};
