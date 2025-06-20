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
    class TcpConnection;

    class TcpListener
    {
    public:
        TcpListener();
        TcpListener(Dispatcher &dispatcher, const Ipv4Address &address, uint16_t port);
        TcpListener(const TcpListener &) = delete;
        TcpListener(TcpListener &&other);
        ~TcpListener();
        TcpListener &operator=(const TcpListener &) = delete;
        TcpListener &operator=(TcpListener &&other);
        TcpConnection accept();

    private:
        Dispatcher *dispatcher;
        int listener;
        void *context;
    };

}
