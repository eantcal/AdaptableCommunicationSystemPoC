//
// This file is part of acsgw
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.
// Licensed under the MIT License.
// See COPYING file in the project root for full license information.
//

/* -------------------------------------------------------------------------- */

#include "TcpSocket.h"
#include "IpAddress.h"

#include <netinet/tcp.h>

/* -------------------------------------------------------------------------- */

TcpSocket &TcpSocket::operator<<(const std::string &text)
{
    send(text);
    return *this;
}

/* -------------------------------------------------------------------------- */

TcpSocket::TcpSocket(const SocketFd &sd,
                     const sockaddr *local_sa,
                     const sockaddr *remote_sa)
    : TransportSocket(sd)
{
    auto conv = [](Port &port, std::string &ip, const sockaddr *sa)
    {
        port = htons(reinterpret_cast<const sockaddr_in *>(sa)->sin_port);
        ip = std::string(
            inet_ntoa(reinterpret_cast<const sockaddr_in *>(sa)->sin_addr));
    };

    conv(_localPort, _localIpAddress, local_sa);
    conv(_remotePort, _remoteHost, remote_sa);
}

/* -------------------------------------------------------------------------- */

bool TcpSocket::connect()
{
    if (!isValid())
        return false;

    IpAddress ipAddress(_remoteHost);
    const Port &port = _remotePort;

    sockaddr_in sin = {0};

    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(ipAddress.to_uint32());
    sin.sin_port = htons(_remotePort);

    struct sockaddr *rsockaddr = reinterpret_cast<struct sockaddr *>(&sin);

    socklen_t sockaddrlen = sizeof(struct sockaddr);

    return (0 == int(::connect(getSocketFd(), rsockaddr, sockaddrlen)));
}

/* -------------------------------------------------------------------------- */

TcpSocket::Handle TcpSocket::make(const std::string &remoteAddr, const Port &port, bool disableTcpDelay)
{
    auto sd = int(::socket(AF_INET, SOCK_STREAM, 0));

    if (sd < 0)
        return nullptr;

    if (disableTcpDelay)
    {
        int flag = 1;
        int result = setsockopt(sd,            /* socket affected */
                                IPPROTO_TCP,   /* set option at TCP level */
                                TCP_NODELAY,   /* name of option */
                                (char *)&flag, /* the cast is historical cruft */
                                sizeof(int));  /* length of option value */

        if (!result)
        {
            perror("setsockopt: disable TCP delay");
        }
    }

    return Handle(new TcpSocket(sd, remoteAddr, port));
}

/* -------------------------------------------------------------------------- */

TcpSocket::Handle TcpSocket::make(bool disableTcpDelay)
{
    auto sd = int(::socket(AF_INET, SOCK_STREAM, 0));

    if (sd < 0)
        return nullptr;

    if (disableTcpDelay)
    {
        int flag = 1;
        int result = setsockopt(sd,            /* socket affected */
                                IPPROTO_TCP,   /* set option at TCP level */
                                TCP_NODELAY,   /* name of option */
                                (char *)&flag, /* the cast is historical cruft */
                                sizeof(int));  /* length of option value */

        if (!result)
        {
            perror("setsockopt: disable TCP delay");
        }
    }

    return Handle(new TcpSocket(sd));
}