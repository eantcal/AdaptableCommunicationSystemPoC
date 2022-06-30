//
// This file is part of acsgw
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.  
// Licensed under the MIT License. 
// See COPYING file in the project root for full license information.
//


/* -------------------------------------------------------------------------- */

#include "SipSocket.h"
#include "Tools.h"

#include <vector>


/* -------------------------------------------------------------------------- */

SipSocket& SipSocket::operator=(TcpSocket::Handle handle)
{
    _socketHandle = handle;
    return *this;
}


/* -------------------------------------------------------------------------- */

SipParsedMessage::Handle SipSocket::recv()
{
    SipParsedMessage::Handle handle(new SipParsedMessage);

    char c = 0;
    int ret = 1;

    enum class CrLfSeq { CR1, LF1, CR2, LF2, IDLE } s = CrLfSeq::IDLE;

    auto crlf = [&s](char c) -> bool {
        switch (s) {
        case CrLfSeq::IDLE:
            s = (c == '\r') ? CrLfSeq::CR1 : CrLfSeq::IDLE;
            break;
        case CrLfSeq::CR1:
            s = (c == '\n') ? CrLfSeq::LF1 : CrLfSeq::IDLE;
            break;
        case CrLfSeq::LF1:
            s = (c == '\r') ? CrLfSeq::CR2 : CrLfSeq::IDLE;
            break;
        case CrLfSeq::CR2:
            s = (c == '\n') ? CrLfSeq::LF2 : CrLfSeq::IDLE;
            break;
        default:
            break;
        }

        return s == CrLfSeq::LF2;
    };

    std::string line;

    while (ret > 0 && _connUp && _socketHandle) {
        std::chrono::seconds sec(getConnectionTimeout());

        auto recvEv = _socketHandle->waitForRecvEvent(sec);

        switch (recvEv) {
        case TransportSocket::RecvEvent::RECV_ERROR:
        case TransportSocket::RecvEvent::TIMEOUT:
            _connUp = false;
            break;
        default:
            break;
        }

        if (!_connUp)
            break;

        ret = _socketHandle->recv(&c, 1);

        if (ret > 0) {
            line += c;
        } else if (ret <= 0) {
            _connUp = false;
            break;
        }

        if (crlf(c)) {
            break;
        }

        if (s == CrLfSeq::LF1) {
            if (!line.empty()) {
                handle->parseHeader(line);
                line.clear();
            }
        }
    }

    return handle;
}


/* -------------------------------------------------------------------------- */

SipSocket& SipSocket::operator<<(const SipResponse& response)
{
    const auto& buf = response.getRespBuffer();

    if (!buf.empty()) {
        if (_socketHandle->send(buf) < 0) {
            _connUp = false;
        }
    }

    return *this;
}

