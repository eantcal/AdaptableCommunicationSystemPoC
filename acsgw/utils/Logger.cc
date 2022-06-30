//
// This file is part of acsgw
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.
// Licensed under the MIT License.
// See COPYING file in the project root for full license information.
//

/* -------------------------------------------------------------------------- */

#include "Logger.h"

#include <syslog.h>
#include <stdio.h>
#include <chrono>

/* -------------------------------------------------------------------------- */

Logger &Logger::getInstance()
{
   static Logger _instance;
   return _instance;
}

/* -------------------------------------------------------------------------- */

void Logger::useStdout(bool setval)
{
   _stdout = setval;
}

/* -------------------------------------------------------------------------- */

void Logger::print(int level, const char *format, ...)
{
   va_list args;
   va_start(args, format);

   if (_stdout)
   {
      using namespace std::chrono;
      milliseconds ms = duration_cast<milliseconds>(
          system_clock::now().time_since_epoch());
      printf("[%lu] ", ms.count());
      vprintf(format, args);
      printf("\n");
   }
   else
   {
      vsyslog(level, format, args);
   }
   va_end(args);
}

/* -------------------------------------------------------------------------- */

void Logger::setLevel(int level)
{
   setlogmask(LOG_UPTO(level));
}

/* -------------------------------------------------------------------------- */

Logger::Logger()
{
   setlogmask(LOG_UPTO(LOG_DEBUG));
   openlog(nullptr, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
}

/* -------------------------------------------------------------------------- */

Logger::~Logger()
{
   closelog();
}
