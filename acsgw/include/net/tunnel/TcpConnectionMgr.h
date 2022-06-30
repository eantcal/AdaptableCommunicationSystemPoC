#pragma once

#include "Exception.h"
#include "Logger.h"
#include "IpAddress.h"
#include "LockedQueue.h"
#include "TcpListener.h"
#include "TcpSocket.h"

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

#define OUTGOING_MSG_QUEUE_LEN 10000
#define INBOUND_MSG_QUEUE_LEN  10000
#define RECV_TIMEOUT          10    // seconds
#define CONNECT_RETRY_INTV    800   // milliseconds
   
/* -------------------------------------------------------------------------- */

class TcpConnectionMgr
{
public:
    using TranspPort = TcpListener::TranspPort;
    using Buffer = std::vector<char>;

    TcpConnectionMgr() = delete;

    TcpConnectionMgr(const IpAddress& localAddr, const TranspPort localPort) :
        _localAddr(localAddr), _localPort(localPort), _server(true)
    {}

    TcpConnectionMgr(
        const IpAddress& localAddr, const TranspPort localPort,
        const IpAddress& remoteAddr, const TranspPort remotePort
    ) : _localAddr(localAddr), _localPort(localPort), _server(false),
        _remoteAddr(remoteAddr), _remotePort(remotePort)
    {}

    const IpAddress& getLocalAddr() const noexcept
    {
        return _localAddr;
    }

    const TranspPort& getLocalPort() const noexcept
    {
        return _localPort;
    }

    const IpAddress& getRemoteAddr() const noexcept
    {
        return _remoteAddr;
    }

    const TranspPort& getRemotePort() const noexcept
    {
        return _remotePort;
    }

    bool recvMessage(Buffer& buf, int timeout) {
        return _inboundMessageQueue.pop(buf, timeout);
    }

    bool sendMessage(Buffer&& buf) {
        return _outgoingMessageQueue.push(std::move(buf));
    }

    bool run();

protected:
    void runConnectionManagerThread();
    int recv(char* buf, int bufSize, int timeoutSec);
    int send(const char* buf, int bufSize);
    void runRecv();

private:
    IpAddress _localAddr;
    TranspPort _localPort;
    bool _server = true;
    IpAddress _remoteAddr;
    TranspPort _remotePort;
    TcpListener::Handle _listener;
    bool _connected = false;
    TcpSocket::Handle _connectionSocket;
    std::unique_ptr<std::thread> _connectionMgrHandle;
    LockedQueue<Buffer> _outgoingMessageQueue{ OUTGOING_MSG_QUEUE_LEN };
    LockedQueue<Buffer> _inboundMessageQueue{ INBOUND_MSG_QUEUE_LEN };

};

/* -------------------------------------------------------------------------- */