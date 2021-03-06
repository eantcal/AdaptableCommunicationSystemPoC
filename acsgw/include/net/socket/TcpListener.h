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

#include "TcpSocket.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <memory>

/* -------------------------------------------------------------------------- */

/**
 * Listens for connections from TCP network clients.
 */
class TcpListener : public TransportSocket
{
public:
    using TranspPort = TcpSocket::Port;
    using Handle = std::unique_ptr<TcpListener>;

    enum class Status
    {
        INVALID,
        VALID
    };

    /**
     * Returns the current state of this connection.
     *
     * @return State::VALID if connection is valid,
     *         State::INVALID otherwise
     */
    Status getStatus() const
    {
        return _status;
    }

    /**
     * Returns the current state of this connection.
     * @return true if connection is valid,
     *         false otherwise
     */
    operator bool() const
    {
        return getStatus() != Status::INVALID;
    }

    /**
     * Returns a handle to a new listener object.
     *
     * @return the handle to a new listenr object instance
     */
    static Handle create()
    {
        return Handle(new TcpListener());
    }

    /**
     * Enables the listening mode, to listen for incoming
     * connection attempts.
     *
     * @param backlog The maximum length of the pending connections queue.
     * @return true if operation successfully completed, false otherwise
     */
    bool listen(int backlog = SOMAXCONN)
    {
        return ::listen(getSocketFd(), backlog) == 0;
    }

    /**
     * Extracts the first connection on the queue of pending connections,
     * and creates a new tcp connection handle
     *
     * @return an handle to a new tcp connection
     */
    TcpSocket::Handle accept();

private:
    std::atomic<Status> _status;

    TcpListener();
};
