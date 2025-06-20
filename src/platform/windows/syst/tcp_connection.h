// Copyright (c) 2012-2017, The MevaCoin developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero Project
// Copyright (c) 2018-2019, The TurtleCoin Developers
// Copyright (c) 2019, The Kryptokrona Developers
//
// Please see the included LICENSE file for more information.

#pragma once

#include <cstdint>
#include <string>

namespace syst
{

    class Dispatcher;
    class Ipv4Address;

    class TcpConnection
    {
    public:
        TcpConnection();
        TcpConnection(const TcpConnection &) = delete;
        TcpConnection(TcpConnection &&other);
        ~TcpConnection();
        TcpConnection &operator=(const TcpConnection &) = delete;
        TcpConnection &operator=(TcpConnection &&other);
        size_t read(uint8_t *data, size_t size);
        size_t write(const uint8_t *data, size_t size);
        std::pair<Ipv4Address, uint16_t> getPeerAddressAndPort() const;

    private:
        friend class TcpConnector;
        friend class TcpListener;

        Dispatcher *dispatcher;
        size_t connection;
        void *readContext;
        void *writeContext;

        TcpConnection(Dispatcher &dispatcher, size_t connection);
    };

}
