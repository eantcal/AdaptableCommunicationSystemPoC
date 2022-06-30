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
#include <map>
#include <string>


/* -------------------------------------------------------------------------- */

void syntax_error(
    const std::string& expr, size_t pos, const std::string& msg = "");


/* -------------------------------------------------------------------------- */

void syntax_error_if(bool cond, const std::string& msg);


/* -------------------------------------------------------------------------- */

void syntax_error_if(bool cond, const std::string& expr, size_t pos,
    const std::string& msg = "");
