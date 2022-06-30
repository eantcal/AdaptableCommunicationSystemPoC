//
// This file is part of acsgw
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.
// Licensed under the MIT License.
// See COPYING file in the project root for full license information.
//

/* -------------------------------------------------------------------------- */

#include "SipSocket.h"
#include "SipClient.h"
#include "TcpSocket.h"
#include "Logger.h"

#include <cassert>
#include <sstream>
#include <chrono>
#include <thread>

/* -------------------------------------------------------------------------- */

SipClient::SipClient(const std::string &remoteAddr,
                     const TransportSocket::Port &remotePort,
                     const std::string &localAddr,
                     const TransportSocket::Port &localPort)
    : _remoteAddr(remoteAddr),
      _remotePort(remotePort),
      _localAddr(localAddr),
      _localPort(localPort)

{
    TRACE(LOG_DEBUG, "[SipClient] connecting to %s:%i", 
        remoteAddr.c_str(), remotePort);

    _connectionMgrHandle = 
        std::make_unique<std::thread>(&SipClient::_connectionMgr, this);

    assert(_connectionMgrHandle);
}

SipParsedMessage::Handle SipClient::sendrecv(std::string &&msg)
{
    TRACE(LOG_DEBUG, "SipClient::sendrecv sending msg %s", msg.c_str());

    int attempts = SEND_RECV_CLIENT_CONNECT_ATTEMPTS;
    while (!_connected && attempts--)  {
        TRACE(LOG_DEBUG, "SipClient::sendrecv waiting for a valid conn to send msg:%s (remaining attempts:%i)", 
            msg.c_str(), attempts);

        if (_stop) {
            TRACE(LOG_DEBUG, "SipClient::sendrecv stopping due to 'stop' request"); 
            break;
        }

        std::this_thread::sleep_for(std::chrono::seconds(CONNECT_RETRY_INTV));
    }

    if (!_connected || _stop || !_connectionMgrHandle)
    {
        TRACE(LOG_DEBUG, 
            "SipClient::sendrecv failed connected:%i stop:%i connectionMgr:%p",
            _connected ? 1 : 0, _stop ? 1 : 0, _connectionMgrHandle.get());

        return nullptr;
    }

    SipParsedMessage::Handle rMsg;

    _sendMsg.push(std::move(msg));

    TRACE(LOG_DEBUG, "SipClient::sendrecv waiting for a response");

    const auto bTimeout = 
        ! _recvMsg.pop(
            rMsg, 
            (RECV_TIMEOUT + 1) * 1000, 
            [&] { return _stop || !_connected; });

    if (bTimeout)
    {
        TRACE(
            LOG_WARNING, 
            "SipClient::sendrecv timeout (stop:%i, connected:%i, rcvMsg:%p)",
            _stop ? 1 : 0, _connected ? 1 : 0, rMsg ? rMsg.get() : nullptr);

        return nullptr;
    }

    std::stringstream ss;
    rMsg->dump(ss);
    std::string line;
    while( std::getline( ss, line ) )
        TRACE(LOG_DEBUG, "SipClient::sendrecv [RECV]:'%s'", line.c_str());

    return std::move(rMsg);
}

SipClient::~SipClient()
{
    if (_connectionMgrHandle)
    {
        _stop = true;
        _connectionMgrHandle->join();
    }
}

void SipClient::_connectionMgr()
{
    while (!_stop)
    {
        TcpSocket::Handle aSocket = TcpSocket::make(_remoteAddr, _remotePort);

        assert(aSocket);

        if (!aSocket)
        {
            TRACE(LOG_ERR, "SipClient::_connectionMgr socket error (bug?)");
            std::this_thread::sleep_for(std::chrono::seconds(SOCKET_RETRY_INTV));
            continue;
        }

        bool bindOK = true;

        if (!_localAddr.empty())
        {
            TRACE(LOG_NOTICE, "SipClient::_connectionMgr binding on %s:%i", _localAddr.c_str(), _localPort);
            bindOK = aSocket->bind(_localAddr, _localPort);
        }
        else if (_localAddr.empty() && _localPort > 0)
        {
            TRACE(LOG_NOTICE, "SipClient::_connectionMgr binding on ANYADDR:%i", _localPort);
            bindOK = aSocket->bind(_localPort);
        }

        if (!bindOK)
        {
            TRACE(LOG_WARNING, "SipClient::_connectionMgr binding failed, retrying");
            std::this_thread::sleep_for(std::chrono::seconds(BIND_RETRY_INTV));
            continue;
        }

        if (!aSocket->connect())
        {
            TRACE(LOG_WARNING, "SipClient::_connectionMgr connect failed, retrying...");
            std::this_thread::sleep_for(std::chrono::seconds(CONNECT_RETRY_INTV));
            continue;
        }

        _connected = true;
        TRACE(LOG_WARNING, "SipClient::_connectionMgr connected to server");

        SipSocket sipSocket(aSocket, RECV_TIMEOUT);

        while (_connected)
        {
            SipParsedMessage::Handle rMsg;

            std::string buf;
            _sendMsg.pop(buf, -1, [&] { return _stop == true; });

            if (_stop)
            {
                TRACE(LOG_NOTICE, "SipClient::_connectionMgr received stop request");
                _connected = false;
                break;
            }

            TRACE(LOG_DEBUG, "SipClient::_connectionMgr sending a message");

            if (_connected && aSocket->send(buf) <= 0)
            {
                TRACE(LOG_ERR, "SipClient::_connectionMgr send failed");
                _connected = false;
                break;
            }

            try
            {
                TRACE(LOG_DEBUG, "SipClient::_connectionMgr waiting for a response");

                sipSocket >> rMsg;

                if (!rMsg)
                {
                    TRACE(LOG_ERR, "SipClient::_connectionMgr failed receiving a response");
                    _connected = false;
                    break;
                }
            }
            catch (...)
            {
                TRACE(LOG_ERR, "SipClient::_connectionMgr failed receiving a response due an exception");
                _connected = false;
                break;
            }

            if (!_recvMsg.push(std::move(rMsg)))
            {
                TRACE(LOG_DEBUG, "SipClient::_connectionMgr cannot process received message");
                _connected = false;
                break;
            }
        }
    }
}
