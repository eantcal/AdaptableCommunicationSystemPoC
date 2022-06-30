//
// This file is part of acsgw
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.  
// Licensed under the MIT License. 
// See COPYING file in the project root for full license information.
//


/* -------------------------------------------------------------------------- */

#include "TcpListener.h"

#include <memory>
#include <string.h>
#include <thread>


/* -------------------------------------------------------------------------- */

TcpListener::TcpListener()
    : TransportSocket(int(::socket(AF_INET, SOCK_STREAM, 0)))
    , _status(isValid() ? Status::VALID : Status::INVALID)
{
}


/* -------------------------------------------------------------------------- */

TcpSocket::Handle TcpListener::accept()
{
    if (getStatus() != Status::VALID)
        return TcpSocket::Handle();

    sockaddr remote_sockaddr = { 0 };

    struct sockaddr* local_sockaddr
        = reinterpret_cast<struct sockaddr*>(&_local_ip_port_sa_in);

    socklen_t sockaddrlen = sizeof(struct sockaddr);

    SocketFd sd = int(::accept(getSocketFd(), &remote_sockaddr, &sockaddrlen));

    TcpSocket::Handle handle = TcpSocket::Handle(sd > 0
            ? new TcpSocket(sd, local_sockaddr, &remote_sockaddr)
            : nullptr);

    return handle;
}

