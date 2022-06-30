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
#include <cstdint>
#include <fstream>
#include <string.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* -------------------------------------------------------------------------- */

class TransportSocket
{

public:
    using Port = uint16_t;
    using SocketFd = int;

    enum class RecvEvent
    {
        RECV_ERROR,
        TIMEOUT,
        RECV_DATA
    };

    using TimeoutInterval = std::chrono::system_clock::duration;

protected:
    TransportSocket(const SocketFd &sd) noexcept
        : _socket(sd)
    {
        memset(&_local_ip_port_sa_in, 0, sizeof(_local_ip_port_sa_in));
    }

public:
    TransportSocket(const TransportSocket &) = delete;
    TransportSocket &operator=(const TransportSocket &) = delete;
    virtual ~TransportSocket();

    bool isValid() const noexcept
    {
        return _socket > 0;
    }

    RecvEvent waitForRecvEvent(const TimeoutInterval &timeout);

    const SocketFd &getSocketFd() const noexcept
    {
        return _socket;
    }

    int send(const char *buf, int len, int flags = 0) noexcept
    {
        return ::send(getSocketFd(), buf, len, flags);
    }

    int recv(char *buf, int len, int flags = 0) noexcept
    {
        return ::recv(getSocketFd(), buf, len, flags);
    }

    int send(const std::string &text) noexcept
    {
        return send(text.c_str(), int(text.size()));
    }

    int sendFile(const std::string &filepath) noexcept;

    bool bind(const std::string &ip, const Port &port)
    {
        if (getSocketFd() <= 0)
        {
            return false;
        }

        sockaddr_in &sin = _local_ip_port_sa_in;
        memset(&sin, 0, sizeof sin);

        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = ip.empty() ? INADDR_ANY : inet_addr(ip.c_str());
        sin.sin_port = htons(port);

        return 0 ==
               ::bind(getSocketFd(), reinterpret_cast<const sockaddr *>(&sin), sizeof(sin));
    }

    bool bind(const Port &port)
    {
        return bind("", port);
    }

private:
    SocketFd _socket = 0;
    enum
    {
        TX_BUFFER_SIZE = 0x100000
    };

protected:
    sockaddr_in _local_ip_port_sa_in;
};
