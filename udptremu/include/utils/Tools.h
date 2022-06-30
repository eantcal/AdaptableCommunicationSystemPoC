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

#include <chrono>
#include <regex>
#include <string>
#include <time.h>
#include <arpa/inet.h>

#if defined(__linux__)
#  include <endian.h>
#elif defined(__FreeBSD__) || defined(__NetBSD__)
#  include <sys/endian.h>
#elif defined(__OpenBSD__)
#  include <sys/types.h>
#  define be16toh(x) betoh16(x)
#  define be32toh(x) betoh32(x)
#  define be64toh(x) betoh64(x)
#endif

#define htonll htobe64
#define ntohll be64toh


/* -------------------------------------------------------------------------- */

namespace Tools {


/* -------------------------------------------------------------------------- */

using TimeoutInterval = std::chrono::system_clock::duration;


/* -------------------------------------------------------------------------- */

/**
 * Converts a timeval object into standard duration object
 *
 * @param d  Duration
 * @param tv Timeval source object
 */
void convertDurationInTimeval(const TimeoutInterval& d, timeval& tv);


/* -------------------------------------------------------------------------- */

/**
 * Gets the system time, corrected for the local time zone
 * Time format is "DoW Mon dd hh:mm:ss yyyy"
 * Example "Thu Sep 19 10:03:50 2013"
 *
 * @param localTime will contain the time
 */
void getLocalTime(std::string& localTime);


/* -------------------------------------------------------------------------- */

/**
 * Gets the system time, corrected for the local time zone
 * Time format is "DoW Mon dd hh:mm:ss yyyy"
 * Example "Thu Sep 19 10:03:50 2013"
 *
 * @return the string will contain the time
 */
inline std::string getLocalTime()
{
    std::string lt;
    getLocalTime(lt);
    return lt;
}


/* -------------------------------------------------------------------------- */

/**
 *  Removes any instances of character c if present at the end of the string s
 *
 *  @param s The input / ouput string
 *  @param c Searching character to remove
 */
void removeLastCharIf(std::string& s, char c);


/* -------------------------------------------------------------------------- */

/**
 * Returns file attributes of fileName.
 *
 * @param fileName String containing the path of existing file
 * @param dateTime Time of last modification of file
 * @param ext File extension or "." if there is no any
 * @return true if operation successfully completed, false otherwise
 */
bool fileStat(const std::string& fileName, std::string& dateTime,
    std::string& ext, size_t& fsize);


/* -------------------------------------------------------------------------- */

/**
 * Splits a text line into a vector of tokens.
 *
 * @param line The string to split
 * @param tokens The vector of splitted tokens
 * @param sep The separator string used to recognize the tokens
 *
 * @return true if operation successfully completed, false otherwise
 */
bool splitLineInTokens(const std::string& line,
    std::vector<std::string>& tokens, const std::string& sep);


} // namespace Tools
