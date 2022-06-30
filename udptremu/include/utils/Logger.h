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

#include <stdarg.h> 
#include <atomic>
#include <syslog.h>

/* -------------------------------------------------------------------------- */

class Logger
{
public:
   static Logger &getInstance();

   void print(int level, const char *format, ...);
 
   void useStdout(bool setval = true);
   static void setLevel(int level);

private:
   Logger();
   ~Logger();

   Logger(const Logger&) = delete;
   Logger& operator=(const Logger&) = delete;

   std::atomic_bool _stdout = false;
};

#ifndef DISABLE_TRACING
#define TRACE(...) Logger::getInstance().print(__VA_ARGS__)
#else
#define TRACE(...)
#endif