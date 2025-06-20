// Copyright (c) 2012-2017, The MevaCoin developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero Project
// Copyright (c) 2018-2019, The TurtleCoin Developers
// Copyright (c) 2019, The Kryptokrona Developers
//
// Please see the included LICENSE file for more information.

#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>

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
        std::size_t read(uint8_t *data, std::size_t size);
        std::size_t write(const uint8_t *data, std::size_t size);
        std::pair<Ipv4Address, uint16_t> getPeerAddressAndPort() const;

    private:
        friend class TcpConnector;
        friend class TcpListener;

        Dispatcher *dispatcher;
        int connection;
        void *readContext;
        void *writeContext;

        TcpConnection(Dispatcher &dispatcher, int socket);
    };

}
