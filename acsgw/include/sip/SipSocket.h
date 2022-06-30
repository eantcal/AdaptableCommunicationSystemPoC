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

#include "SipParsedMessage.h"
#include "SipResponse.h"

#include <string>


/* -------------------------------------------------------------------------- */
// SipResponse


/* -------------------------------------------------------------------------- */

/**
 * This class represents an SIP connection between a client and a server.
 */
class SipSocket {
private:
    TcpSocket::Handle _socketHandle;
    bool _connUp = true;
    SipParsedMessage::Handle recv();
    int _connectionTimeOut = 120; // secs

public:
    SipSocket(const SipSocket&) = default;

    /**
     * Construct the SIP connection starting from TCP connected-socket handle.
     */
    SipSocket(TcpSocket::Handle handle, int connectionTimeout = 120)
        : _socketHandle(handle), 
        _connectionTimeOut(connectionTimeout)
    {
    }

    /**
     * Assigns a new TCP connected socket handle to this SIP socket.
     */
    SipSocket& operator=(TcpSocket::Handle handle);

    /**
     * Returns TCP socket handle
     */
    operator TcpSocket::Handle() const { 
       return _socketHandle; 
    }

    /**
     * Receives an SIP message from remote peer.
     * @param the handle of sip message object
     */
    SipSocket& operator>>(SipParsedMessage::Handle& handle) {
        handle = recv();
        return *this;
    }

    /**
     * Returns false if last recv/send operation detected
     * that connection was down; true otherwise.
     */
    explicit operator bool() const { 
       return _connUp; 
    }

    /**
     * Send a response to remote peer.
     * @param response The SIP response
     */
    SipSocket& operator<<(const SipResponse& response);


    /**
     * Send a response to remote peer.
     * @param response The SIP response
     */
    int sendFile(const std::string& fileName) {
        return _socketHandle->sendFile(fileName);
    }

    /*
     * Return connection timeout interval in seconds
     */
    const int& getConnectionTimeout() const noexcept {
        return _connectionTimeOut;
    }
};
