/* -------------------------------------------------------------------------- */

#include "TcpConnectionMgr.h"
#include <list>


/* -------------------------------------------------------------------------- */

bool TcpConnectionMgr::run()
{
    if (_server)
    {
        TRACE(LOG_DEBUG, "TcpConnectionMgr::run() TCP SERVER mode");
        _listener = TcpListener::create();

        if (_localAddr.operator int() == 0)
        {
            if (!_listener->bind(getLocalPort()))
            {
                perror("bind");
                return false;
            }
        }
        else
        {
            if (!_listener->bind(getLocalAddr().to_str(), getLocalPort()))
            {
                perror("bind");
                return false;
            }
        }

        assert(_listener);

        _listener->listen();
    }
    else
    {
        TRACE(LOG_DEBUG, "TcpConnectionMgr::run() TCP CLIENT mode");
    }

    _connectionMgrHandle =
        std::make_unique<std::thread>(&TcpConnectionMgr::runConnectionManagerThread, this);

    return true;
}

/* -------------------------------------------------------------------------- */

int TcpConnectionMgr::recv(char *buf, int bufSize, int timeoutSec)
{
    if (!buf || bufSize < 0)
        return -1;

    if (bufSize == 0)
        return 0;

    const char *threadType = _server ? "recv[Server]:" : "recv[Client]:";

    int left = bufSize;
    int offset = 0;

    while (left > 0)
    {
        std::chrono::seconds sec(timeoutSec);
        auto recvEv = _connectionSocket->waitForRecvEvent(sec);

        bool timeout = false;
        switch (recvEv)
        {
        case TransportSocket::RecvEvent::RECV_ERROR:
            TRACE(LOG_ERR, "%s [%p] TcpConnectionMgr::recv error", threadType, this);
            return -1;

        case TransportSocket::RecvEvent::RECV_DATA:
            break;

        default:
        case TransportSocket::RecvEvent::TIMEOUT:
            TRACE(LOG_DEBUG, "%s [%p] TcpConnectionMgr::recv timeout", threadType, this);
            return offset;
        }

        int rbytes = _connectionSocket->recv(buf + offset, left);

        if (rbytes == 0)
            return offset;

        if (rbytes < 0)
            return -1;

        if (rbytes < left)
        {
            offset += rbytes;
            left -= rbytes;
            continue;
        }

        break;
    }

    return bufSize;
}


/* -------------------------------------------------------------------------- */

int TcpConnectionMgr::send(const char *buf, int bufSize)
{
    if (!buf || bufSize < 0)
        return -1;

    if (bufSize == 0)
        return 0;

    int left = bufSize;
    int offset = 0;

    while (left > 0)
    {
        int wbytes = _connectionSocket->send(buf + offset, left);

        if (wbytes == 0)
            return offset;

        if (wbytes < 0)
            return -1;

        if (wbytes < left)
        {
            offset += wbytes;
            left -= wbytes;
            continue;
        }

        break;
    }

    return bufSize;
}

/* -------------------------------------------------------------------------- */

void TcpConnectionMgr::runRecv()
{
    const char *threadType = _server ? "runRecv[Server]:" : "runRecv[Client]:";
    TRACE(LOG_DEBUG, "%s [%p] TcpConnectionMgr::runRecv+", threadType, this);

    while (_connected)
    {
        uint32_t len = 0;
        int res = recv((char *)&len, sizeof(len), RECV_TIMEOUT);

        if (res == 0)
            continue;

        if (res < sizeof(len))
        {
            TRACE(LOG_ERR, "%s [%p] TcpConnectionMgr::runRecv cannot read message header", threadType, this);
            _connected = false;
        }

        len = ntohl(len);

        TRACE(LOG_DEBUG, "%s [%p] TcpConnectionMgr::runRecv len=%u", threadType, this, len);

        if (len > 0 && len < (128 * 1024))
        {
            Buffer buf(len + 8);
            if (recv((char *)buf.data(), len + 8, RECV_TIMEOUT) < len)
            {
                TRACE(LOG_ERR, "%s [%p] TcpConnectionMgr::runRecv len=%u failed", threadType, this, len);
                _connected = false;
            }
            else {
                _inboundMessageQueue.push(std::move(buf));
            }
        }
        else
        {
            TRACE(LOG_ERR, "%s [%p] TcpConnectionMgr::runRecv wrong len %ul", threadType, this, len);
            _connected = false;
        }
    }

    TRACE(LOG_DEBUG, "%s [%p] TcpConnectionMgr::runRecv-", threadType, this);
}

/* -------------------------------------------------------------------------- */

void TcpConnectionMgr::runConnectionManagerThread()
{
    const char *threadType = _server ? "runConnectionManagerThread[Server]:" : "runConnectionManagerThread[Client]:";
    std::list<Buffer> retryQueue;
    bool bindOK = false;
    while (1)
    {
        if (!_server) // client
        {
            if (!bindOK) 
            {
                _connectionSocket = TcpSocket::make(_remoteAddr.to_str(), _remotePort);
            
                assert(_connectionSocket);

                if (_localAddr.operator int() == 0)
                { 
                    if (!_connectionSocket->bind(getLocalPort()))
                    {
                        perror("bind");
                        TRACE(LOG_ERR, "%s [%p] TcpConnectionMgr::runConnectionManagerThread bind on port %i error (bug?)", threadType, this, getLocalPort());
                        std::this_thread::sleep_for(std::chrono::milliseconds(CONNECT_RETRY_INTV));
                        continue;
                    }
                    bindOK = true;
                }
                else
                {
                    const auto lAddr = getLocalAddr().to_str();
                    if (!_connectionSocket->bind(lAddr, getLocalPort()))
                    {
                        perror("bind");
                        TRACE(LOG_ERR, "%s [%p] TcpConnectionMgr::runConnectionManagerThread bind on %s:%i error (bug?)", threadType, this,
                              lAddr.c_str(), getLocalPort());
                        std::this_thread::sleep_for(std::chrono::milliseconds(CONNECT_RETRY_INTV));
                        continue;
                    }
                    bindOK = true;
                }
            }

            if (!_connectionSocket->connect())
            {
                const auto rAddr = getRemoteAddr().to_str();
                TRACE(LOG_ERR, "%s [%p] TcpConnectionMgr::runConnectionManagerThread could not connect to %s:%i, retrying...", 
                   threadType, this, rAddr.c_str(), getRemotePort());
                
                std::this_thread::sleep_for(std::chrono::milliseconds(CONNECT_RETRY_INTV));
                continue;
            }
        }
        else 
        {

            TRACE(LOG_DEBUG, "%s [%p] TcpConnectionMgr::runConnectionManagerThread _connectionSocket = _listener->accept()", threadType, this);
            _connectionSocket = _listener->accept();

            assert (_connectionSocket);
        }

        _connected = true;

        TRACE(LOG_WARNING, "%s [%p] TcpConnectionMgr::runConnectionManagerThread connected to server", threadType, this);

        auto receiverThread = std::make_unique<std::thread>(&TcpConnectionMgr::runRecv, this);
        assert (receiverThread);

        while (_connected)
        {
            Buffer buf;
            if (!retryQueue.empty())
            {
                buf = retryQueue.front();
            }
            else if (!_outgoingMessageQueue.pop(buf, RECV_TIMEOUT * 1000, [&]() { return !_connected; }))
            {
                if (!_connected)
                   break;

                TRACE(LOG_DEBUG, "%s [%p] TcpConnectionMgr::runConnectionManagerThread pop timeout (continue)", threadType, this);
                continue;
            }

            // receive a buffer with room for 4 bytes on the top (to put the msg len) 
            uint32_t len = htonl(buf.size() - 4 - 8);

            // put the message len on top of the message
            memcpy(buf.data(), (char*) &len, 4);
            
            TRACE(LOG_DEBUG, "%s [%p] TcpConnectionMgr::runConnectionManagerThread sending a message", threadType, this);

            // send len + msg in one shot
            if (send(buf.data(), buf.size()) != buf.size())
            {
                if (retryQueue.empty()) 
                {
                    retryQueue.push_front(buf);
                }
                TRACE(LOG_ERR, "%s [%p] TcpConnectionMgr::runConnectionManagerThread cannot send the message", threadType, this);
                _connected = false;
                break;
            }
            else {
                if (!retryQueue.empty())
                {
                    retryQueue.pop_front();
                }
            }
        }

        TRACE(LOG_DEBUG, "%s [%p] TcpConnectionMgr::runConnectionManagerThread connection DOWN", threadType, this);

        bindOK = false;

        receiverThread->join();
        _connectionSocket->shutdown();
        _connectionSocket = nullptr;

        TRACE(LOG_DEBUG, "%s [%p] TcpConnectionMgr::runConnectionManagerThread receiverThread->join() completed", threadType, this);

    }
}
