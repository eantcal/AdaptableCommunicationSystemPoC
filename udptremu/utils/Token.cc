//  
// This file is part of acsgw
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.  
// Licensed under the MIT License. 
// See COPYING file in the project root for full license information.
//

/* -------------------------------------------------------------------------- */

#include "Token.h"


/* -------------------------------------------------------------------------- */

std::string Token::getDescription(TokenClass tc)
{
    std::string desc;

    switch (tc) {
    case TokenClass::BLANK:
        desc = "BLNK";
        break;

    case TokenClass::INTEGRAL:
        desc = "INTG";
        break;

    case TokenClass::NEWLINE:
        desc = "NEWL";
        break;

    case TokenClass::REAL:
        desc = "REAL";
        break;

    case TokenClass::OPERATOR:
        desc = "OPER";
        break;

    case TokenClass::SUBEXP_BEGIN:
        desc = "BGNE";
        break;

    case TokenClass::SUBEXP_END:
        desc = "ENDE";
        break;

    case TokenClass::STRING_LITERAL:
        desc = "STRL";
        break;

    case TokenClass::STRING_COMMENT:
        desc = "COMM";
        break;

    case TokenClass::IDENTIFIER:
        desc = "IDNT";
        break;

    case TokenClass::UNDEFINED:
    default:
        desc = "UNDF";
        break;
    }

    return desc;
}


/* -------------------------------------------------------------------------- */

void Token::set_identifier(const std::string& id, case_t casemode)
{
    _identifier = id;
    _org_id = id;

    switch (casemode) {
    case case_t::LOWER:
        _transformLoCase();
        break;

    case case_t::NOCHANGE:
    default:
        // do nothing
        break;
    }
}


/* -------------------------------------------------------------------------- */

Token::Token(const std::string& id, TokenClass t, size_t pos,
    DataPtr expr_ptr) noexcept : 
        _identifier(id),
        _org_id(id),
        _type(t),
        _position(pos),
        _expression_ptr(expr_ptr)
{
    _transformLoCase();
}
