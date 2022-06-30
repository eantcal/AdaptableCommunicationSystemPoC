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

#include "StringTool.h"
#include <exception>
#include <string>


/* -------------------------------------------------------------------------- */

class Exception : public std::exception {
public:
    Exception() = delete;
    Exception(const Exception&) = default;
    Exception& operator=(const Exception&) = default;

    /** ctor
      *  @param message C-style string error message.
      */
    explicit Exception(const char* message)
        : _msg(message)
    {
    }

    //! move ctor
    Exception(Exception&& e) noexcept { 
        _msg = std::move(e._msg); 
    }

    //! move assign operator
    Exception& operator=(Exception&& e) noexcept {
        if (&e != this)
            _msg = std::move(e._msg);

        return *this;
    }

    /** ctor
     *  @param message The error message.
     */
    explicit Exception(const std::string& message)
        : _msg(message)
    {
    }

    /** dtor
     * Virtual to allow for subclassing.
     */
    virtual ~Exception() throw() {
    }

    /** Returns a pointer to the (constant) error description.
     *  @return A pointer to a const char*.
     *          The underlying memory is in posession of the Exception
     *          object. Callers must not free the memory.
     */
    const char* what() const throw() override { 
        return _msg.c_str(); 
    }

protected:
    /** Error message.
     */
    std::string _msg;
};
