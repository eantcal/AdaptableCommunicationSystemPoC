//
// This file is part of udptremu
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.
// Licensed under the MIT License.
// See COPYING file in the project root for full license information.
//

/* -------------------------------------------------------------------------- */

#include "Exception.h"
#include "Tools.h"
#include "UdpSocket.h"

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
#include <fstream>
#include <vector>
#include <stdio.h>
#include <time.h>
#include <set>

/* -------------------------------------------------------------------------- */

struct Config
{
   uint32_t firstId = 1;
   uint32_t lastId = 100;
   uint16_t interval = 1000; // ms
   uint16_t pktSize = 128;
   uint32_t statsTimeout = 5; // sec
   uint32_t ontimelimit = 1500; // ms
};

class PktPF
{
public:
#pragma pack(push, 1)
   struct Header
   {
      uint64_t timestamp;
      uint64_t epochid;
      uint32_t ident;
      uint32_t firstId;
      uint32_t lastId;
      uint16_t interval;
      uint16_t pktSize;
   };
#pragma pack(pop)

   static uint64_t epoch() {
      static uint64_t epochId = timestamp();
      return epochId;
   }

   using Buffer = std::vector<char>;

   PktPF(const Config &cfg = Config()) : _firstId(cfg.firstId)
                                       , _lastId(cfg.lastId)
                                       , _interval(cfg.interval)
                                       , _pktSize(cfg.pktSize)
   {
   }

   Buffer forgeNewPacket(uint32_t id)
   {
      const int paddingSize = _pktSize - sizeof(Header) - 28;

      assert(paddingSize >= 0);

      Buffer pkt(_pktSize);

      auto *pktPtr = reinterpret_cast<Header *>(pkt.data());
      pktPtr->timestamp = htonll(timestamp());
      pktPtr->epochid = htonll(epoch());
      pktPtr->ident = htonl(id);
      pktPtr->firstId = htonl(_firstId);
      pktPtr->lastId = htonl(_lastId);
      pktPtr->interval = htons(_interval);
      pktPtr->pktSize = htons(_pktSize);

      return pkt;
   }

   bool parse(const Buffer &pkt, Header &header)
   {
      if (pkt.size() < sizeof(header))
      {
         return false;
      }

      const auto *pktPtr = reinterpret_cast<const Header *>(pkt.data());

      header.timestamp = htonll(pktPtr->timestamp);
      header.epochid = htonll(pktPtr->epochid);
      header.ident = htonl(pktPtr->ident);
      header.firstId = htonl(pktPtr->firstId);
      header.lastId = htonl(pktPtr->lastId);
      header.interval = htons(pktPtr->interval);
      header.pktSize = htons(pktPtr->pktSize);

      return true;
   }

   static uint64_t timestamp()
   {
      using namespace std::chrono;
      milliseconds ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
      return ms.count();
   }

private:
   uint32_t _firstId = 1;
   uint32_t _lastId = 100;
   uint16_t _interval = 1000;
   uint16_t _pktSize = 128;
};

/* -------------------------------------------------------------------------- */

class ProgArgs
{
private:
   std::string _progName;
   std::string _cmdLine;
   std::string _cfgFileName = "tremu.cfg";

   std::string _localAddr;
   std::string _serverAddr;

   uint16_t _localPort = 0;
   uint16_t _serverPort = 54321;

   uint32_t _count = 100;
   uint16_t _interval = 1000;
   uint16_t _pktSize = 128;
   uint32_t _firstId = 1;
   uint32_t _statsTimeout = 5; //sec
   uint32_t _onTimeLimit = 1500; //msec
   uint32_t _latencyLimit = 1500; //msec

   bool _showHelp = false;
   bool _showVersion = false;
   bool _error = false;
   bool _verbose = false;
   bool _serverMode = false;

   std::string _errMsg;

   static constexpr int _min_ver = 1;
   static constexpr int _maj_ver = 0;

public:
   ProgArgs() = delete;

   const std::string &getProgName() const noexcept
   {
      return _progName;
   }

   const std::string &getCmdLine() const noexcept
   {
      return _cmdLine;
   }

   const std::string &getLocalAddr() const noexcept
   {
      return _localAddr;
   }

   const std::string &getServerAddr() const noexcept
   {
      return _serverAddr;
   }

   uint16_t getLocalPort() const noexcept
   {
      return _localPort;
   }

   uint16_t getServerPort() const noexcept
   {
      return _serverPort;
   }

   uint16_t getPktSize() const noexcept
   {
      return _pktSize;
   }

   uint32_t getFirstId() const noexcept
   {
      return _firstId;
   }

   uint32_t getInterval() const noexcept
   {
      return _interval;
   }

   uint32_t getStatsTimeout() const noexcept
   {
      return _statsTimeout;
   }

   uint32_t getOnTimeLimit() const noexcept
   {
      return _onTimeLimit;
   }

   uint32_t getLatencyLimit() const noexcept
   {
      return _latencyLimit;
   }

   uint32_t getCount() const noexcept
   {
      return _count;
   }

   const std::string &getCfgFileName() const
   {
      return _cfgFileName;
   }

   bool isOK() const
   {
      return !_error;
   }

   bool verbose() const
   {
      return _verbose;
   }

   bool serverMode() const
   {
      return _serverMode;
   }

   const std::string &error() const
   {
      return _errMsg;
   }

   bool show(std::ostream &os) const
   {
      if (_showVersion)
         os << getProgName() << " " << _maj_ver << "." << _min_ver << "\n";

      if (!_showHelp)
         return _showVersion;

      os << "Usage:\n";
      os << "\t" << getProgName() << "\n";
      os << "\t\t-S | --servermode\n";
      os << "\t\t\tStart as server (client otherwise)\n";
      os << "\t\t-b | --localaddr <ip=[any]>\n";
      os << "\t\t\tLocal bind address (any by default)\n";
      os << "\t\t-l | --localport <port=[generated]>\n";
      os << "\t\t\tLocal bind port (assigned by O/S for client, 54321 by default for server mode, otherwise)\n";
      os << "\t\t-r | --serveraddr <ip>\n";
      os << "\t\t\tServer address (for server mode it is equivalent to local address)\n";
      os << "\t\t-p | --serverport <port=54321>\n";
      os << "\t\t\tServer port (for server mode it is equivalent to local port)\n";
      os << "\t\t-c | --count <number=100>\n";
      os << "\t\t\tNumber of packets to send\n";
      os << "\t\t-i | --interval <milliseconds=1000>\n";
      os << "\t\t\tInterval between on tx packet and the next one\n";
      os << "\t\t-s | --size <byte=128>\n";
      os << "\t\t\tIP packet size \n";
      os << "\t\t-x | --ident <firstid=1>\n";
      os << "\t\t\tFirst packet identifier (1 by default) in a epoch\n";
      os << "\t\t-si | --statsintvl <seconds=5>\n";
      os << "\t\t\tNo more packets timeout.Time the server wait for last packet of an epoch before printing out results\n";
      os << "\t\t-np | --ontimelimit <millisecons=1500>\n";
      os << "\t\t\tMax threshold in ms consider a packet valid since last received packet (the packet arrived too late)\n";
      os << "\t\t-np | --latencylimit <millisecons=1500>\n";
      os << "\t\t\tMax latency value to consider a packet still valid (the packet arrived too late)\n";
      os << "\t\t-vv | --verbose\n";
      os << "\t\t\tVerbose mode (for debugging purposes)\n";
      os << "\t\t-v | --version\n";
      os << "\t\t-h | --help\n";

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
         SRVPORT,
         SRVADDR,
         LOCALADDR,
         LOCALPORT,
         COUNT,
         INTERVAL,
         STATSHOLDINTVL,
         ONTIMELIMIT,
         LATENCYLIMIT,
         SIZE,
         FIRSTID,
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
            if (sarg == "--serverport" || sarg == "-p")
            {
               state = State::SRVPORT;
            }
            else if (sarg == "--serveraddr" || sarg == "-r")
            {
               state = State::SRVADDR;
            }
            else if (sarg == "--localport" || sarg == "-l")
            {
               state = State::LOCALPORT;
            }
            else if (sarg == "--localaddr" || sarg == "-b")
            {
               state = State::LOCALADDR;
            }
            else if (sarg == "--count" || sarg == "-c")
            {
               state = State::COUNT;
            }
            else if (sarg == "--interval" || sarg == "-i")
            {
               state = State::INTERVAL;
            }
            else if (sarg == "--statsintvl" || sarg == "-si")
            {
               state = State::STATSHOLDINTVL;
            }
            else if (sarg == "--ontimelimit" || sarg == "-ol")
            {
               state = State::ONTIMELIMIT;
            }
            else if (sarg == "--latencylimit" || sarg == "-ll")
            {
               state = State::LATENCYLIMIT;
            }
            else if (sarg == "--size" || sarg == "-s")
            {
               state = State::SIZE;
            }
            else if (sarg == "--ident" || sarg == "-x")
            {
               state = State::FIRSTID;
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
            else if (sarg == "--verbose" || sarg == "-vv")
            {
               _verbose = true;
               state = State::OPTION;
            }
            else if (sarg == "--servermode" || sarg == "-S")
            {
               _serverMode = true;
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

         case State::LOCALADDR:
            _localAddr = sarg;
            state = State::OPTION;
            break;

         case State::LOCALPORT:
            _localPort = std::stoi(sarg) & 0xffffL;
            state = State::OPTION;
            break;

         case State::SRVADDR:
            _serverAddr = sarg;
            state = State::OPTION;
            break;

         case State::SRVPORT:
            _serverPort = std::stoi(sarg) & 0xffffL;
            state = State::OPTION;
            break;

         case State::SIZE:
            _pktSize = std::stol(sarg) & 0xffffffffLL;
            state = State::OPTION;
            break;

         case State::COUNT:
            _count = std::stol(sarg) & 0xffffffffLL;
            state = State::OPTION;
            break;

         case State::FIRSTID:
            _firstId = std::stol(sarg) & 0xffffffffLL;
            state = State::OPTION;
            break;

         case State::INTERVAL:
            _interval = std::stoi(sarg) & 0xffffL;
            state = State::OPTION;
            break;

         case State::STATSHOLDINTVL:
            _statsTimeout = std::stoi(sarg) & 0xffffL;
            state = State::OPTION;
            break;

         case State::ONTIMELIMIT:
            _onTimeLimit = std::stoi(sarg) & 0xffffL;
            state = State::OPTION;
            break;

         case State::LATENCYLIMIT:
            _latencyLimit = std::stoi(sarg) & 0xffffL;
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
   ProgArgs _progArgs;

   struct Stat
   {
      Stat(const PktPF::Header &header_, int64_t recvTimestamp_) : header(header_), recvTimestamp(recvTimestamp_)
      {
      }

      PktPF::Header header;
      int64_t recvTimestamp = 0;
   };

   std::map<uint32_t, Stat> _stats;
   std::multimap<uint32_t, Stat> _stats_with_duplicated;

   using Stats = std::map<uint32_t, Stat>;

public:
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

      if (_progArgs.verbose()) {
         printf("%s is starting at %s\n\n",
                _progArgs.getProgName().c_str(),
                Tools::getLocalTime().c_str());
      }
   }

   void runServer(const Config &cfg)
   {
      if (_progArgs.verbose())
      {
         printf("Server+\n");
      }
      PktPF p(cfg);
      UdpSocket udpSocket;

      bind(udpSocket);

      PktPF::Buffer payload(65536);
      IpAddress src;
      UdpSocket::PortType port;
      uint64_t epochId = 0;
      uint64_t lastepochId = 0;

      while (1)
      {
         struct timeval timeval;
         timeval.tv_sec = _progArgs.getStatsTimeout();
         timeval.tv_usec = 0;

         auto pollRes = udpSocket.poll(timeval);

         if (pollRes == UdpSocket::PollingState::TIMEOUT_EXPIRED)
         {
            if (_progArgs.verbose())
            {
               printf("Waiting for packets\n");
            }
            if (!_stats.empty() && lastepochId != epochId) // this identify a bunch of packets sent togheter (for statistics)
            {
               printStats(_stats); // this ensures to reset counter first time server start
               
               //if (_progArgs.verbose()) {
               //   printStats(_stats_with_duplicated); // this ensures to reset counter first time server start
               //}
               _stats.clear();
               _stats_with_duplicated.clear();
               lastepochId = epochId;
            }
            continue;
         }
         else if (pollRes == UdpSocket::PollingState::ERROR_IN_COMMUNICATION)
         {
            perror("Error receiving packet\n");
            break;
         }
//#define SIMULATE_LATENCY
#ifdef SIMULATE_LATENCY
         std::this_thread::sleep_for(std::chrono::milliseconds(random() % 1000) ); // TODO remove this
#endif
         auto recvTimestamp = PktPF::timestamp();

         if (udpSocket.recvfrom(payload.data(), payload.size(), src, port) < 0)
         {
            perror("Error receiving packet\n");
            break;
         }

         if (_progArgs.verbose())
         {
            printf("Recv pkt (%i) from %s:%i\n", int(payload.size()), src.to_str().c_str(), int(port));
         }
         else {
            printf(".");
            fflush(stdout);
         }

         PktPF::Header header;
         PktPF p;
         p.parse(payload, header);

// #define SIMULATE_PACKET_LOSS
#ifdef SIMULATE_PACKET_LOSS
         if (random() & 1)  { // TODO remove this
#endif
            _stats.insert({header.ident, Stat(header, recvTimestamp)});
            _stats_with_duplicated.insert({header.ident, Stat(header, recvTimestamp)});
#ifdef SIMULATE_PACKET_LOSS
         }
#endif
         epochId = header.epochid;
      }

      if (_progArgs.verbose())
      {
         printf("Server-\n");
      }
   }

   // ----------------------------------------------

   //template<class Stats>
   void printStats(Stats & stats)
   {
      int64_t old_rx_timestamp = 0;
      int64_t old_tx_timestamp = 0;
      int64_t interval = 0;

      if ((void*) &stats == (void*) &_stats_with_duplicated) 
      {
         printf("STATISTICS which includes any DUPLICATED packets (debug only)\n");
      }

      printf("\n%10s %10s %10s %10s %10s\n", "Ident", "Tx-Timestamp", "Rx-Timestamp", "Latency (ms)", "Interval (ms)");
      int64_t lost = 0;
      double jitter = 0;

      uint32_t firstId = 0;
      uint32_t lastId = 0;
      uint32_t expectedPktCount = 0;
      uint32_t pktCount = 0;
      int64_t expectedInterval = 0;
      int64_t firstTimestamp = 0;
      uint32_t dupCount = 0;
      std::set<uint32_t> idset;
      bool firstentry = true;
      uint32_t outOfTimeMessage = 0;
      uint32_t outOfOrderMessage = 0;
      uint32_t delayedMessage = 0;
      uint64_t latencyTotal = 0;

      for (const auto &row : stats)
      {
         const auto &header = row.second.header;
         int64_t latency = row.second.recvTimestamp - header.timestamp;

         if (!idset.insert(header.ident).second) {
            ++dupCount; 
         }

         if (firstentry)
         {
            old_rx_timestamp = row.second.recvTimestamp;
            old_tx_timestamp = header.timestamp;
            firstId = header.firstId;
            lastId = header.lastId;
            expectedPktCount = lastId - firstId + 1;
            pktCount = 1;
            expectedInterval = header.interval;
            jitter = 0;
            firstTimestamp = header.timestamp;
            firstentry = false;
            latencyTotal = latency;
         }
         else
         {
            interval = int64_t(row.second.recvTimestamp) - old_rx_timestamp;
            jitter += interval;
            if (row.second.recvTimestamp<old_rx_timestamp)
            {
               ++outOfOrderMessage;
            }
            old_tx_timestamp = header.timestamp;
            old_rx_timestamp = row.second.recvTimestamp;;
            ++pktCount;
            latencyTotal += latency;
         }

         time_t epch = header.timestamp;

         bool outOfTimeFrameFlag = false;
         bool highLatencyFlag = false;

         if (interval>_progArgs.getOnTimeLimit())
         {
            ++outOfTimeMessage;
            outOfTimeFrameFlag = true;
         }

         if (latency>_progArgs.getLatencyLimit())
         {
            ++delayedMessage;
            highLatencyFlag = true;
         }

         const std::string onTime(outOfTimeFrameFlag ?     "(delayed)" : "(on-time)");
         const std::string onTimeLatency(highLatencyFlag ? "(delayed)" : "(on-time)");

         const auto relativeTxTS = header.timestamp - firstTimestamp;
         printf("%10i %10li %10li %10li %s %10li %s\n", header.ident, relativeTxTS, relativeTxTS+latency, latency, onTimeLatency.c_str(), interval, onTime.c_str());
      }

      // Compute the jitter
      if (pktCount > 1)
      {
         jitter /= double(pktCount-1);
         jitter = abs(expectedInterval-jitter);
      }
      else 
      {
         jitter = .0;
      }
      
      const int32_t pktLost = expectedPktCount - pktCount;
      const double lostRate = double(pktLost) / double(expectedPktCount);

      printf("\n\tExpected_Packtes: %i\n", expectedPktCount);
      printf("\tRx_Packet_Count: %i\n\n", pktCount);

      printf("\tPackets_Lost: %i\n", pktLost);
      printf("\tLost_Rate: %.2f %%\n\n", lostRate * 100.0);

      printf("\tOver_%i_ms: %i\n",_progArgs.getOnTimeLimit(), outOfTimeMessage);
      printf("\tOver_%i_ms_Rate: %.2f %%\n\n",_progArgs.getOnTimeLimit(), (double(outOfTimeMessage)/double(pktCount))*100.0);

      printf("\tLatency_over_%i_ms: %i\n",_progArgs.getLatencyLimit(), delayedMessage);
      printf("\tLatency_over_%i_ms_Rate: %.2f %%\n\n",_progArgs.getLatencyLimit(), (double(delayedMessage)/double(pktCount))*100.0);

      printf("\tOut_of_order: %i\n", outOfOrderMessage);
      printf("\tOut_of_order_Rate: %.2f %%\n\n", (double(outOfOrderMessage)/double(pktCount)) * 100.0);

      const auto totFail = outOfOrderMessage + outOfTimeMessage;
      printf("\tTotal_Fails: %i\n", totFail);
      printf("\tTotal_Fails_Rate: %.2f %%\n\n", (double(totFail)/double(pktCount)) * 100.0);
      printf("\tDuplicated (discarded): %i\n", uint32_t(_stats_with_duplicated.size())-pktCount);
      //printf("\tDuplicated_Rate: %.2f %%\n\n", (double(dupCount)/double(pktCount)) * 100.0);
      printf("\tAvg_Latency: %.2f ms\n", double(latencyTotal)/double(pktCount));
      printf("\tJitter: %.2f ms\n", jitter);
   }

   // ----------------------------------------------

   void bind(UdpSocket &udpSocket)
   {
      UdpSocket::PortType port = _progArgs.getLocalPort();
      bool anyAddr = false;
      IpAddress localIp(_progArgs.getLocalAddr());

      if (_progArgs.serverMode())
      {
         if (port == 0)
         {
            port = _progArgs.getServerPort();
         }

         if (localIp.to_uint32() == 0)
         {
            if (_progArgs.getServerAddr().empty())
            {
               anyAddr = true;
            }
            else
            {
               localIp = IpAddress(_progArgs.getServerAddr());
            }
         }
      }
      if (_progArgs.getLocalAddr().empty() || anyAddr)
      {
         if (!udpSocket.bind(port))
         {
            std::stringstream ss;
            ss << "Error binding to " << _progArgs.getLocalPort();
            throw Exception(ss.str());
         }
      }
      else
      {

         if (!udpSocket.bind(port, localIp))
         {
            std::stringstream ss;
            ss << "Error binding to " << _progArgs.getLocalAddr() << ":" << _progArgs.getLocalPort();
            throw Exception(ss.str());
         }
      }
   }

   void runClient(const Config &cfg)
   {
      UdpSocket udpSocket;

      if (!_progArgs.getLocalAddr().empty() || _progArgs.getLocalPort() > 0)
         bind(udpSocket);

      PktPF p(cfg);
      const auto dest = _progArgs.getServerAddr();
      const IpAddress destIp(dest);

      for (uint32_t i = cfg.firstId; i <= cfg.lastId; ++i)
      {
         auto aPkt = p.forgeNewPacket(i);

         if (_progArgs.verbose())
         {
            printf("%4i) [%i] -> %s:%i\n", i, int(aPkt.size()), dest.c_str(), int(_progArgs.getServerPort()));
         }
         else
         {
            printf(".");
            fflush(stdout);
         }
         if (udpSocket.sendto(aPkt.data(), aPkt.size(), destIp, _progArgs.getServerPort()) < aPkt.size())
         {
            std::stringstream ss;
            ss << "Error sending pkt to " << _progArgs.getServerAddr() << ":" << _progArgs.getServerPort();
            throw Exception(ss.str());
         }

         std::this_thread::sleep_for(std::chrono::milliseconds(cfg.interval));
      }

      printf("\nSent %i packets to %s:%i\n", _progArgs.getCount(), dest.c_str(), int(_progArgs.getServerPort()));
   }

   void run()
   {
      if (_progArgs.getServerAddr().empty() && !_progArgs.serverMode())
      {
         throw Exception("Server address is missing");
      }

      if (_progArgs.getServerPort() == 0)
      {
         throw Exception("Server port cannot be 0");
      }

      Config cfg;

      cfg.firstId = _progArgs.getFirstId();
      cfg.lastId = _progArgs.getFirstId() + _progArgs.getCount() - 1;
      cfg.interval = _progArgs.getInterval();
      cfg.pktSize = _progArgs.getPktSize();
      cfg.statsTimeout = _progArgs.getStatsTimeout();
      cfg.ontimelimit = _progArgs.getOnTimeLimit();

      if (_progArgs.serverMode())
      {
         std::cout << "Server started" << std::endl;
         std::cout << "Listening on " << _progArgs.getServerAddr() << ":" << _progArgs.getServerPort() << std::endl;
         std::cout << "Statistics Timout " << cfg.statsTimeout << " seconds" << std::endl;
         std::cout << "On time packet threshold " << cfg.ontimelimit << " ms" << std::endl;
         std::cout << "Latency Limit " << _progArgs.getLatencyLimit() << " ms" << std::endl;
         std::cout << std::endl;

         runServer(cfg);
      }
      else
      {
         std::cout << "Client mode" << std::endl;
         std::cout << "FirstId       = " << cfg.firstId << std::endl;
         std::cout << "LastId        = " << cfg.lastId << std::endl;
         std::cout << "Count         = " << _progArgs.getCount() << std::endl;
         std::cout << "Interval (ms) = " << cfg.interval << std::endl;
         std::cout << "FirstId       = " << cfg.firstId << std::endl;
         std::cout << std::endl;

         runClient(cfg);
      }
   }
};

/* -------------------------------------------------------------------------- */

/**
 * Program entry point
 */

int main(int argc, char *argv[])
{
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

   return 0;
}
