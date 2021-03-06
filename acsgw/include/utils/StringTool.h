//  
// This file is part of acsgw
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.  
// Licensed under the MIT License. 
// See COPYING file in the project root for full license information.
//

/* -------------------------------------------------------------------------- */

#ifndef __NU_STRING_TOOL_H__
#define __NU_STRING_TOOL_H__


/* -------------------------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <string>


/* -------------------------------------------------------------------------- */


//! Parses s interpreting its content as a floating-point number
//! which is returned as a value of type float

inline float stof(const std::string& s) { 
    return (float)atof(s.c_str()); 
}


/* -------------------------------------------------------------------------- */

//! Parses s interpreting its content as a floating-point number
//! which is returned as a value of type float
inline double stod(const std::string& s) { 
    return std::stod(s); 
}


/* -------------------------------------------------------------------------- */

//! Parses s interpreting its content as an integral number
//! which is returned as an int value
inline int stoi(const std::string& s) { 
    return std::stoi(s); 
}


/* -------------------------------------------------------------------------- */

//! Parses s interpreting its content as an long integral number
//! which is returned as an int value
inline long long stoll(const std::string& s) { 
    return std::stoll(s); 
}


/* -------------------------------------------------------------------------- */

//! Returns a string with the representation of item
template <class T> inline std::string to_string(const T& item) {
    return std::to_string(item);
}


/* -------------------------------------------------------------------------- */

#if _MSC_VER
#define strcasecmp _stricmp
#endif // _MSC_VER


/* -------------------------------------------------------------------------- */

#endif //__NU_STRING_TOOL_H__
