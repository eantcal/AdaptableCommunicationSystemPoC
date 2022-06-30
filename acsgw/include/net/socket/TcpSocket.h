//
// This file is part of acsgw
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.  
// Licensed under the MIT License. 
// See COPYING file in the project root for full license information.
//


#pragma once


/* -------------------------------------------------------------------------- */

#include "TransportSocket.h"

#include <memory>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>

/* -------------------------------------------------------------------------- */

/**
 * This class represents a TCP connection between a client and a server
 */
class TcpSocket : public TransportSocket {
    friend class TcpListener;
    friend class TcpClient;

public:
    enum class shutdown_mode_t : int {
        DISABLE_RECV,
        DISABLE_SEND,
        DISABLE_SEND_RECV
    };

    using Handle = std::shared_ptr<TcpSocket>;

    TcpSocket(const TcpSocket&) = delete;
    TcpSocket& operator=(const TcpSocket&) = delete;
    ~TcpSocket() = default;

    const std::string& getLocalIpAddress() const {
        return _localIpAddress;
    }

    const Port& getLocalPort() const {
        return _localPort;
    }

    const std::string& getRemoteHost() const {
        return _remoteHost;
    }

    const Port& getRemotePort() const {
        return _remotePort;
    }
    
    int shutdown(shutdown_mode_t how = shutdown_mode_t::DISABLE_SEND_RECV) {
        return ::shutdown(getSocketFd(), static_cast<int>(how));
    }

    bool connect();

    // Create a new socket
    static Handle make(bool disableTcpDelay = true);

    // Create a new client socket
    static Handle make(const std::string& remoteAddr, const Port& Port, bool disableTcpDelay = true);

    // Sends a string on this socket
    TcpSocket& operator<<(const std::string& text);

    TcpSocket() = delete;
    

private:
    std::string _localIpAddress;
    Port _localPort = 0;
    std::string _remoteHost;
    Port _remotePort = 0;

    TcpSocket(
        const SocketFd& sd, 
        const sockaddr* local_sa,
        const sockaddr* remote_sa);

    TcpSocket(const SocketFd& sd) : 
        TransportSocket(sd)
    {
    }

    TcpSocket(const SocketFd& sd, 
              const std::string& remoteHost, 
              const Port& port) :
        TransportSocket(sd),
        _remoteHost(remoteHost),
        _remotePort(port)
    {}
};