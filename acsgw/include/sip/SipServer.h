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

#include "Config.h"
#include "SipSocket.h"
#include "SipClient.h"
#include "TcpListener.h"
#include "TunnelBuilder.h"

#include <iostream>
#include <string>

/* -------------------------------------------------------------------------- */

/**
 * The top-level class of the SIP server.
 */
class SipServer
{
public:
    using TranspPort = TcpListener::TranspPort;
    enum
    {
        DEFAULT_PORT = 15060
    };

private:
    TranspPort _bindPort = DEFAULT_PORT;
    std::string _bindAddress; // if not specified the bind is to any address
    std::string _remoteProxy;
    TransportSocket::Port _remotePort = 0;

    TcpListener::Handle _tcpServer;
    Config::Handle _config;
    SipClient::Handle _proxyEndPoint;
    std::string _dnsHostsFilename;
    std::string _dnsHostsInitContent;

public:
    using Handle=std::shared_ptr<SipServer>;

    SipServer(Config::Handle config);

    SipServer(const SipServer &) = delete;
    SipServer &operator=(const SipServer &) = delete;

    /**
     * Gets SipServer object instance reference.
     * This class is a singleton. First time this function is called, 
     * the SipServer object is initialized.
     *
     * @return the SipServer reference
     */
    static auto getInstance() -> SipServer &;

    /**
     * Sets the port where server is listening
     */
    void setLocalPort(int port) noexcept
    {
        _bindPort = port;
    }

    const TranspPort getLocalPort() const noexcept
    {
        return _bindPort;
    }

    /**
     * Binds the SIP server to a local TCP port
     *
     * @param port listening port
     * @return true if operation is successfully completed, false otherwise
     */
    bool bind();

    /**
     * Sets the server in listening mode
     *
     * @param maxConnections back log list length
     * @return true if operation is successfully completed, false otherwise
     */
    bool listen(int maxConnections);

    /**
     * Runs the server. This function is blocking for the caller.
     */
    void run();


protected:
    /**
     * Accepts a new connection from a remote client.
     * This function blocks until connection is established or
     * an error occurs.
     * @return a handle to tcp socket
     */
    TcpSocket::Handle accept()
    {
        return _tcpServer->accept();
    }
};
