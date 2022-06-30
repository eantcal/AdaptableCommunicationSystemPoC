//
// This file is part of acsgw
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.
// Licensed under the MIT License.
// See COPYING file in the project root for full license information.
//

/* -------------------------------------------------------------------------- */

#include "Exception.h"
#include "GlobalDefs.h"
#include "ConfigParser.h"
#include "SipServer.h"
#include "Tools.h"
#include "MpTunnel.h"
#include "TunnelBuilder.h"
#include "LogicalIpAddrMgr.h"
#include "Logger.h"

#include <cassert>
#include <iostream>
#include <memory>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <future>
#include <array>
#include <mutex>
#include <chrono>
#include <queue>
#include <map>
#include <signal.h>
#include <cerrno>

/* -------------------------------------------------------------------------- */

class ProgArgs
{
private:
   std::string _progName;
   std::string _cmdLine;
   std::string _cfgFileName = SIP_SERVER_CFGFNAME;

   TcpSocket::Port _sipLocalPort = 0; // 0 means not assigned

   bool _showHelp = false;
   bool _showVersion = false;
   bool _error = false;
   bool _logOnStdOut = false;

   std::string _errMsg;

   static const int _min_ver = SIP_SERVER_MIN_V;
   static const int _maj_ver = SIP_SERVER_MAJ_V;

public:
   ProgArgs() = delete;

   const std::string &getProgName() const
   {
      return _progName;
   }

   const std::string &getCmdLine() const
   {
      return _cmdLine;
   }

   TcpSocket::Port getSipLocalPort() const
   {
      return _sipLocalPort;
   }

   const std::string &getCfgFileName() const
   {
      return _cfgFileName;
   }

   bool isOK() const
   {
      return !_error;
   }

   bool logOnStdout() const
   {
      return _logOnStdOut;
   }

   const std::string &error() const
   {
      return _errMsg;
   }

   bool show(std::ostream &os) const
   {
      if (_showVersion)
         os << SIP_SERVER_NAME << " " << _maj_ver << "." << _min_ver
            << "\n";

      if (!_showHelp)
         return _showVersion;

      os << "Usage:\n";
      os << "\t" << getProgName() << "\n";
      os << "\t\t-p | --port <port>\n";
      os << "\t\t\tBind server to a TCP port number (default is "
         << SIP_SERVER_PORT << ") \n";
      os << "\t\t-w | --wdir <working_dir_path>\n";
      os << "\t\t-c | --config <config file name>\n";
      os << "\t\t\tSpecify the configuration file name (default is "
         << SIP_SERVER_CFGFNAME << ") \n";
      os << "\t\t-vv | --logstdout\n";
      os << "\t\t\tEnable logging on stdout\n";
      os << "\t\t-v | --version\n";
      os << "\t\t\tShow software version\n";
      os << "\t\t-h | --help\n";
      os << "\t\t\tShow this help \n";

      return true;
   }

   /* -------------------------------------------------------------------------- */

   /**
    * Parse the command line
    */
   ProgArgs(int argc, char *argv[])
   {
      if (!argc)
         return;

      _progName = argv[0];
      _cmdLine = _progName;

      if (argc <= 1)
         return;

      enum class State
      {
         OPTION,
         PORT,
         CFG
      } state = State::OPTION;

      for (int idx = 1; idx < argc; ++idx)
      {
         std::string sarg = argv[idx];

         _cmdLine += " ";
         _cmdLine += sarg;

         switch (state)
         {
         case State::OPTION:
            if (sarg == "--port" || sarg == "-p")
            {
               state = State::PORT;
            }
            else if (sarg == "--config" || sarg == "-c")
            {
               state = State::CFG;
            }
            else if (sarg == "--help" || sarg == "-h")
            {
               _showHelp = true;
               state = State::OPTION;
            }
            else if (sarg == "--version" || sarg == "-v")
            {
               _showVersion = true;
               state = State::OPTION;
            }
            else if (sarg == "--logstdout" || sarg == "-vv")
            {
               _logOnStdOut = true;
               state = State::OPTION;
            }
            else
            {
               _errMsg = "Unknown option '" + sarg + "', try with --help or -h";
               _error = true;
               return;
            }
            break;

         case State::CFG:
            _cfgFileName = sarg;
            state = State::OPTION;
            break;

         case State::PORT:
            _sipLocalPort = std::stoi(sarg);
            state = State::OPTION;
            break;
         }
      }
   }
};

/* --------------------------------------------------------------------------- */

struct Program
{
private:

   struct Log
   {
      Log()
      {
         Logger::getInstance();
      }
   } setUpLogger;

   struct SysHandlers
   {
      SysHandlers()
      {
         installSignalHandlers();
      }
   } setupSysHandlers;

   static Config::Handle readConfig(const std::string cfgFileName)
   {
      std::ifstream is(cfgFileName);

      if (!is.is_open())
      {
         std::string err = "Error opening " + cfgFileName;
         throw Exception(err);
      }

      std::string configBuf(
          (std::istreambuf_iterator<char>(is)),
          std::istreambuf_iterator<char>());

      Config::Handle cfgHandle;

      try
      {
         ConfigParser parser;
         ConfigTnkzr tknzr(configBuf, 0);
         cfgHandle = parser.parse(tknzr);

         if (cfgHandle)
         {
            std::stringstream ss;
            cfgHandle->compile();
            cfgHandle->describe(ss);
            std::string line;
            while (std::getline(ss, line))
            {
               TRACE(LOG_DEBUG, "|C| %s", line.c_str());
            }
         }
      }
      catch (std::exception &e)
      {
         throw Exception(e.what());
      }
      catch (...)
      {
         throw Exception("Cannot read configuration");
      }

      return cfgHandle;
   }

   static TunnelBuilder::Handle makeTunnelBuilder(Config &cfg)
   {
      auto tunnelBuilder = std::make_shared<TunnelBuilder>(cfg);

      assert(tunnelBuilder);

      if (!tunnelBuilder || !tunnelBuilder->buildTunnels())
      {
         if (getuid() == 0)
            throw Exception("Cannot build the tunnels");
      }

      return tunnelBuilder;
   }

   static SipServer::Handle makeSipServer(
       Config::Handle cfgHandle,
       TunnelBuilder::LookupTblHandler lktblHandle,
       ProgArgs &args)
   {
      auto sipServer = SipServer::Handle(new SipServer(cfgHandle));

      if (!sipServer)
      {
         throw Exception("Cannot create SIP server instance");
      }

      if (args.getSipLocalPort() > 0)
         sipServer->setLocalPort(args.getSipLocalPort());

      int bindMaxAttempts = 60;
      bool res = false;
      while (!(res = sipServer->bind()) && bindMaxAttempts-- > 0)
      {
         perror("Bind failed, retrying. Probable cause:");
         sleep(5);
      }

      if (!res)
      {
         throw Exception("Sip server cannt bind to the local port");
      }

      res = sipServer->listen(SIP_SERVER_BACKLOG);
      if (!res)
      {
         throw Exception(std::strerror(errno));
      }

      return sipServer;
   }

   static void ctrlCHandler(int s)
   {
      std::cerr << " - Shutting down the server..." << std::endl;
      exit(1);
   }

   static void installSignalHandlers()
   {
      signal(SIGINT, ctrlCHandler);
      signal(SIGPIPE, SIG_IGN);
   }

   ProgArgs _progArgs;
   Config::Handle _cfgHandle;
   SipServer::Handle _sipServer;
   TunnelBuilder::Handle _tunnelBuilder;

public:
   ~Program()
   {
      closelog();
   }

   Program(int argc, char **argv) : _progArgs(argc, argv)
   {
      if (!_progArgs.isOK())
      {
         throw Exception(_progArgs.error());
      }

      if (_progArgs.show(std::cout))
      {
         // if --help or --version is required
         exit(0);
      }

      Logger::getInstance().useStdout(_progArgs.logOnStdout());

      TRACE(LOG_DEBUG, "%s is starting at %s", 
         _progArgs.getProgName().c_str(), 
         Tools::getLocalTime().c_str());
      
      _cfgHandle = readConfig(_progArgs.getCfgFileName());

      if (!_cfgHandle)
      {
         throw Exception("Invalid configuration or configuration not found");
      }

      // Configure the logical ip addresses manager
      LogicalIpAddrMgr::getInstance().configure(*_cfgHandle);

      _tunnelBuilder = makeTunnelBuilder(*_cfgHandle);

      _sipServer =
          makeSipServer(_cfgHandle, _tunnelBuilder->getTunnelsTbl(), _progArgs);
   }

   void run()
   {
      assert(_sipServer);

      auto sipServerThread =
          std::async(std::launch::async, &SipServer::run, _sipServer.get());

      sipServerThread.wait();
   }
};

/* -------------------------------------------------------------------------- */

/**
 * Program entry point
 */

int main(int argc, char *argv[])
{
   if (getuid())
   {
      // Warning notice
      std::cerr << "Notice: this program requires root privileges." << std::endl;
   }

   try
   {
      Program thisProgram(argc, argv);
      thisProgram.run();
   }
   catch (Exception &e)
   {
      std::cerr << e.what() << std::endl;
      return 1;
   }
   catch (std::exception &e)
   {
      std::cerr << e.what() << std::endl;
      return 1;
   }
   catch (...)
   {
      std::cerr << "Unexpected fatal error" << std::endl;
      return 1;
   }

#if 0
   close(0); // close stdin
   close(1); // close stout
   close(2); // close stderr
   open("/dev/null",O_RDWR); 
   dup(0);   // redirect stdout and stderr to null
   dup(0);
#endif
   return 0;
}
