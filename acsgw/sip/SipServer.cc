//
// This file is part of acsgw
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.
// Licensed under the MIT License.
// See COPYING file in the project root for full license information.
//

/* -------------------------------------------------------------------------- */
// SIP Server
/* -------------------------------------------------------------------------- */

#include "SipServer.h"
#include "Tools.h"
#include "TextTokenizer.h"
#include "LogicalIpAddrMgr.h"
#include "RouteMgr.h"
#include "Logger.h"

#include <thread>
#include <cassert>
#include <iostream>
#include <string>

/* -------------------------------------------------------------------------- */
// SipServerTask

/* -------------------------------------------------------------------------- */

class SipServerTask
{
private:
    TcpSocket::Handle _tcpSocketHandle;
    std::string _remoteProxy;
    int _remotePort = 0;
    SipClient::Handle _sipProxyHandle;
    RouteMgr _routeMgr;
    std::string _dnsHostsFilename;
    std::string _dnsHostsInitContent;

    TcpSocket::Handle &getTcpSocketHandle()
    {
        return _tcpSocketHandle;
    }

    SipServerTask(
        TcpSocket::Handle socketHandle,
        const std::string &remoteProxy,
        const int &remotePort,
        const std::string& dnsHostsFilename,
        const std::string& dnsHostsInitContent)
        : _tcpSocketHandle(socketHandle),
          _remoteProxy(remoteProxy),
          _remotePort(remotePort),
          _dnsHostsFilename(dnsHostsFilename),
          _dnsHostsInitContent(dnsHostsInitContent)
    {
        if (!remoteProxy.empty())
            _sipProxyHandle = std::make_shared<SipClient>(remoteProxy, remotePort);
    }

    SipResponse::Handle processRequest(const SipParsedMessage &message);
    SipResponse::Handle processResponse(const SipParsedMessage &message);

    SipParsedMessage::Handle forwardRequest(const SipParsedMessage &message);

    IpAddress generateIp(const std::string &name)
    {
        auto &resolver = LogicalIpAddrMgr::getInstance();
        IpAddress ip;
        resolver.generateIp(name, ip);
        return ip;
    }

    IpAddress resolveIp(const std::string &name)
    {
        auto &resolver = LogicalIpAddrMgr::getInstance();
        IpAddress ip;
        resolver.resolveIp(name, ip);
        return ip;
    }

    bool updateDns() 
    {
        if (_dnsHostsFilename.empty())
            return false;

        std::ofstream hosts(_dnsHostsFilename);

        if (hosts.is_open())
        {
            hosts << _dnsHostsInitContent << std::endl;

            auto &resolver = LogicalIpAddrMgr::getInstance();
            hosts << resolver.getHosts() << std::endl;

            hosts.flush();
            hosts.close();

            return ::system("sudo systemctl reload dnsmasq")==0;
        }

        return false;
    }

    std::string getCommandLine(const SipParsedMessage &message, const std::string &msgtype)
    {
        const auto &hl = message.getParsedHeaderMap();
        auto it = hl.find(msgtype);
        if (it != hl.end())
        {
            const auto &data = it->second;
            const auto &kvm = data->kvm();
            auto kvmit = kvm.find("host");
            if (kvmit != kvm.end())
                return kvmit->second;
        }
        return std::string();
    }

    std::string getHeader(const SipParsedMessage &message, const std::string &name)
    {
        const auto &hl = message.getParsedHeaderMap();
        auto it = hl.find(name);
        if (it != hl.end())
        {
            const auto &data = it->second;
            const auto &kvm = data->kvm();
            if (!kvm.empty())
            {
                const std::string &val = kvm.begin()->second;
                return val.empty() ? kvm.begin()->first : val;
            }
        }

        return std::string();
    }

    bool isAllowed(const SipParsedMessage &message, const std::string &name) const
    {
        const auto &hl = message.getParsedHeaderMap();

        auto it = hl.find("allow");

        if (it != hl.end())
        {
            const auto &data = it->second;
            const auto &kvm = data->kvm();
            return !kvm.empty() && kvm.find(name) != kvm.end();
        }

        return false;
    }

public:
    using Handle = std::shared_ptr<SipServerTask>;

    inline static Handle create(
        TcpSocket::Handle socketHandle,
        const std::string &remoteProxy,
        const int &remotePort,
        const std::string& dnsHostsFilename,
        const std::string& dnsHostsInitContent)
    {
        return Handle(new SipServerTask(
            socketHandle,
            remoteProxy,
            remotePort,
            dnsHostsFilename,
            dnsHostsInitContent));
    }

    SipServerTask() = delete;
    void operator()(Handle task_handle);
};

/* -------------------------------------------------------------------------- */

SipResponse::Handle SipServerTask::processRequest(const SipParsedMessage &message)
{
    const bool thisIsNetworkGateway = _remoteProxy.empty();

    auto sipServerName = getCommandLine(message, "register");

    if (sipServerName.empty())
        sipServerName = getCommandLine(message, "invite");

    auto to = getCommandLine(message, "to");
    auto tunnelId = getHeader(message, "x-tunnel");
    auto serverType = isAllowed(message, "invite");
    auto from = getHeader(message, "from");
    auto contact = getHeader(message, "contact");
    auto methodName = message.getMethodName();

    TRACE(LOG_DEBUG, "SipServerTask::processRequest(%s) "
                     "methodName:%s sipServerName:%s "
                     "serverAgent:%s tunnelId:%s userType:%s "
                     "from:%s contact:%s",
          thisIsNetworkGateway ? "NG" : "OG",
          methodName.c_str(),
          sipServerName.c_str(),
          to.c_str(),
          tunnelId.c_str(),
          serverType ? "Server" : "Client",
          from.c_str(),
          contact.c_str());

    if (thisIsNetworkGateway)
    {

        // expected message for registration:
        //
        // REGISTER sip:{sipServerName} SIP/2.0
        //  Contact: <sip:{useragent}@{sipServerName}>
        //  From: <sip:{useragent}@{sipServerName}>
        //  CSeq: 1 REGISTER
        //  X-Tunnel: {tunnel-id}
        //  Content-Length: 0
        //
        TRACE(LOG_DEBUG, "SipServerTask::processRequest NG (%s)", methodName.c_str());

        // REGISTER
        if (methodName == "register" && (!contact.empty() || !from.empty()))
        {
            std::string registeringName = contact.empty() ? from : contact;
            
            IpAddress ip = generateIp(contact);

            if (ip.to_int()==0 || !updateDns()) 
            {
                TRACE(LOG_ERR, "SipServerTask::processRequest NG (%s) can't update DNS server", methodName.c_str());
            }

            // if this is a useragent request and we are network gateway (we have received the request via tunnel i/f)
            if (!serverType)
            {
                TRACE(LOG_DEBUG, "SipServerTask::processRequest from useragent %s", registeringName.c_str());

                // tunnelId must be not null
                if (tunnelId.empty())
                {
                    return std::make_shared<SipResponse>(message, 400);
                }

                // add local route to ip via tunnelid
                _routeMgr.add(ip, tunnelId);

                //  SIP/2.0 200 OK
                //  CSeq: 1 REGISTER
                //  Contact: <sip:{useragent}@{sipServerName}>
                //  Expires: {expires} // todo
                //  Content-length: 0
                return std::make_shared<SipResponse>(message, 200);
            }
            // if this is a serveragent request we are configured as network gateway
            // the request comes from local i/f, so we add a route to remote address which
            // the request comes from.
            else
            {
                TRACE(LOG_DEBUG, "SipServerTask::processRequest from serveragent %s", registeringName.c_str());

                if (tunnelId.empty())
                {
                    return std::make_shared<SipResponse>(message, 400);
                }

                // add local route to ip via local interface (to remote address)
                _routeMgr.add(ip, IpAddress(_tcpSocketHandle->getRemoteHost()));

                //  SIP/2.0 200 OK
                //  CSeq: 1 REGISTER
                //  Contact: <sip:{useragent}@{sipServerName}>
                //  Expires: {expires} // todo
                //  Content-length: 0
                return std::make_shared<SipResponse>(message, 200);
            }
        }
        // We are configured as a NG, and a useragent INVITEs a serveragent
        else if (methodName == "invite")
        {
            if (from.empty() && !contact.empty())
                from = contact;

            if (from.empty() || to.empty())
            {
                return std::make_shared<SipResponse>(message, 400);
            }

            TRACE(LOG_DEBUG, "SipServerTask::processRequest Invite to:%s from:%s", to.c_str(), from.c_str());

            // INVITE {sip:sipserver} SIP/2.0
            //  CSeq: 1 INVITE
            //  From: <sip:{useragent}@{sipServerName}>
            //  To: <sip:{serveragent}@{sipServerName}>
            //  X-Tunnel: {tunnel-id}
            //  Content-Length: 0
            // auto response = message; // copy original message if you need to modify it

            // SIP/2.0 200 OK
            //  CSeq: 1 INVITE
            //  From: <sip:{useragent}>
            //  To: <sip:{serveragent}
            //  X-Tunnel: {tunnel-id}
            //  Expires: {expires} // todo
            //  Content-Length: 0
            return std::make_shared<SipResponse>(message, 200);
        }
    }
    else // We are acting as OG (this is a response from NG)
    {
        return std::make_shared<SipResponse>(message, 405); // method not allowed
    }

    return std::make_shared<SipResponse>(message, 400); // bad request
}

/* -------------------------------------------------------------------------- */

SipResponse::Handle SipServerTask::processResponse(const SipParsedMessage &message)
{
    const bool thisIsNetworkGateway = _remoteProxy.empty();

    auto registerName = getCommandLine(message, "register");
    auto serverAgent = getCommandLine(message, "invite");

    auto from = getHeader(message, "from");
    auto to = getHeader(message, "to");
    auto allow = getHeader(message, "allow");

    if (!thisIsNetworkGateway)
    {
        auto contact = getHeader(message, "contact");
        auto tunnelId = getHeader(message, "x-tunnel");

        // SIP/2.0 200 OK
        // Contact: <sip:useragent>
        // X-Tunnel: tunnel-id
        // Content-Length: 0

        if (!contact.empty())
        {
            IpAddress ip = generateIp(contact);
            if (ip.to_int() && message.isResposeOK())
            {
                updateDns();
                _routeMgr.add(ip, IpAddress(_tcpSocketHandle->getRemoteHost())); // disable for now
            }
        }
        // SIP/2.0 200 OK
        //  CSeq: 1 INVITE
        //  From: <sip:{useragent}>
        //  To: <sip:{serveragent}>
        //  X-Tunnel: {tunnel-id}
        //  Expires: {expires} // todo
        //  Content-Length: 0
        else if (!to.empty() && !tunnelId.empty() && message.isResposeOK())
        {
            // We want to set the route to reach out the serveragent via tunnel i/f
            _routeMgr.add(IpAddress(to), tunnelId);
        }

        return std::make_shared<SipResponse>(message, message.getResponseCode());
    }

    return std::make_shared<SipResponse>(message, 400); // bad request
}

/* -------------------------------------------------------------------------- */

SipParsedMessage::Handle SipServerTask::forwardRequest(const SipParsedMessage &message)
{
    TRACE(LOG_DEBUG, "SipServerTask::forwardRequest forwading to remote server %s", 
        message.getMethodName().c_str());

    if (_sipProxyHandle)
    {
        const auto &headers = message.getHeaderList();
        if (!headers.empty())
        {
            std::string buf;

            for (const auto &header : headers)
                buf += header;

            buf += "\r\n";

            return _sipProxyHandle->sendrecv(std::move(buf));
        }
    }
    else
    {
        TRACE(LOG_DEBUG, 
            "SipServerTask::forwardRequest not forwading to remote server %s,"
            " since remote host not configured", message.getMethodName().c_str());
    }

    return nullptr;
}

/* -------------------------------------------------------------------------- */

// Handles the SIP server request
// This method executes in a specific thread context for
// each accepted SIP request
void SipServerTask::operator()(Handle task_handle)
{
    (void)task_handle;

    const int sd = getTcpSocketHandle()->getSocketFd();

    TRACE(LOG_NOTICE, "SipServerTask(sd=%i) started", sd);

    // Generates an identifier for recognizing the transaction
    auto transactionId = [sd]() {
        return "[" + std::to_string(sd) + "] " + "[" + Tools::getLocalTime() + "]";
    };

    while (getTcpSocketHandle())
    {
        // Create an SIP socket around a connected tcp socket
        SipSocket sipSocket(getTcpSocketHandle());

        // Wait for a request from remote peer
        SipParsedMessage::Handle sipRequest;
        sipSocket >> sipRequest;

        // If an error occoured terminate the task
        if (!sipSocket)
        {
            TRACE(LOG_ERR, "SipServerTask is closing the connection [%s]", transactionId().c_str());
            break;
        }

#if 0 // DBG onlyF
        {
            std::stringstream ss;
            sipRequest->dump(ss, transactionId());
            for (auto header : sipRequest->getHeaderList())
            {
                rtrim(header);
                TRACE(LOG_DEBUG, "SipServerTask RQST:%s", header.c_str());
            }
        }
#endif
        // if a remote server address is configured the
        // request will be forwarded
        auto rMsg = forwardRequest(*sipRequest);

        SipResponse::Handle response;

        // If we get an answer from remote server we process it
        if (rMsg)
        {
            TRACE(LOG_DEBUG, "SipServer received a response from remote server");
            response = processResponse(*rMsg);
        }
        else
        {
            TRACE(LOG_DEBUG, "SipServer is processing a request locally (remote proxy not set)");
            response = processRequest(*sipRequest);
        }

        assert(response);

        if (response)
        {
            sipSocket << *response;

            TRACE(LOG_DEBUG, "SipServerTask Response:%s", response->getRespBuffer().c_str());
        }
    }

    getTcpSocketHandle()->shutdown();

    TRACE(LOG_NOTICE, "SipServerTask(sd=%i) terminated", sd);
}

/* -------------------------------------------------------------------------- */
// SipServer

SipServer::SipServer(Config::Handle config) : _config(config)
{
    bool proxy = false;

    auto getNum = [&](
                      const Config::ConfigNamespacedParagraph &nsp,
                      const std::string &fieldName,
                      int minVal, int maxVal,
                      TranspPort &retVal) {
        auto nsit = nsp.find(fieldName);
        if (nsit != nsp.end())
        {
            try
            {
                int n = std::stoi(nsit->second.first);
                if (n >= minVal && n <= maxVal)
                {
                    retVal = TranspPort(n);
                }
            }
            catch (...)
            {
                TRACE(LOG_DEBUG, "SipServer config syntax error in %s format", fieldName.c_str());
            }
        }
    };

    if (config)
    {
        const auto &cfgdata = config->data();

        // [sip] //
        auto it = cfgdata.find("sip");
        if (it != cfgdata.end())
        {
            const auto &namespace_data = it->second;
            // local_port = x
            getNum(namespace_data, "local_port", 1, 65535, _bindPort);
            getNum(namespace_data, "remote_port", 1, 65535, _remotePort);

            // local_addres = "x.x.x.x"
            auto nsit = namespace_data.find("local_address");
            if (nsit != namespace_data.end())
            {
                _bindAddress = nsit->second.first; // if empty will bind to anyaddr
            }

            // remote_address = "x.x.x.x"
            nsit = namespace_data.find("remote_address"); // remote proxy
            if (nsit != namespace_data.end())
            {
                _remoteProxy = nsit->second.first;
            }
        }

        it = cfgdata.find("dns");
        if (it != cfgdata.end())
        {
            const auto &namespace_data = it->second;

            for (const auto &[key, value] : namespace_data)
            {
                if (key == "hosts")
                {
                    _dnsHostsFilename = value.first;
                }
                else
                {
                    _dnsHostsInitContent += value.first + "\n";
                }
            }

            if (!_dnsHostsFilename.empty())
            {
                std::ofstream outfile(_dnsHostsFilename);
                if (!outfile.is_open()) {
                    TRACE(LOG_ERR, "SipServer failed creating %s", _dnsHostsFilename.c_str());
                }
                else if (!_dnsHostsInitContent.empty())
                {
                    TRACE(LOG_DEBUG, "SipServer creating initial content for %s", _dnsHostsFilename.c_str());
                    outfile << _dnsHostsInitContent;

                    auto res = ::system("sudo systemctl restart dnsmasq"); // TODO improve it
                    if (!res) {
                        TRACE(LOG_ERR, "Failed restarting dnsmasq", _dnsHostsFilename.c_str());
                    }
                }
            }
        }
    }

    if (_remotePort > 0 && !_remoteProxy.empty())
    {
        _proxyEndPoint = std::make_shared<SipClient>(_remoteProxy, _remotePort);
    }
}

/* -------------------------------------------------------------------------- */

bool SipServer::bind()
{
    _tcpServer = TcpListener::create();

    assert(_tcpServer);

    if (!_tcpServer)
    {
        TRACE(LOG_DEBUG, "SipServer failed creating a tcp listener");
        return false;
    }

    TRACE(LOG_DEBUG, "SipServer binding on %s:%i",
          _bindAddress.c_str(), _bindPort);

    if (!_tcpServer->bind(_bindAddress, _bindPort))
    {
        TRACE(LOG_ERR, "SipServer cannot bind on %s:%i",
              _bindAddress.c_str(), _bindPort);
        return false;
    }

    return true;
}

/* -------------------------------------------------------------------------- */

bool SipServer::listen(int maxConnections)
{
    assert(_tcpServer);

    if (!_tcpServer)
    {
        TRACE(LOG_DEBUG, "listen() is failing since listener object is null");
        return false;
    }

    return _tcpServer->listen(maxConnections);
}

/* -------------------------------------------------------------------------- */

void SipServer::run()
{
    // Create a thread for each TCP accepted connection and
    // delegate it to handle SIP request / response
    while (true)
    {
        const TcpSocket::Handle handle = accept();

        // Fatal error: we stop the server
        if (!handle)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        SipServerTask::Handle taskHandle = SipServerTask::create(
            handle, _remoteProxy, _remotePort, 
            _dnsHostsFilename, _dnsHostsInitContent);

        // Coping the sip_server_task handle (shared_ptr) the reference
        // count is automatically increased by one
        std::thread workerThread(*taskHandle, taskHandle);

        workerThread.detach();
    }
}
