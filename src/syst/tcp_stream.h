// Copyright (c) 2012-2017, The MevaCoin developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero Project
// Copyright (c) 2018-2019, The TurtleCoin Developers
// Copyright (c) 2019, The Kryptokrona Developers
//
// Please see the included LICENSE file for more information.

#pragma once

#include <array>
#include <cstdint>
#include <streambuf>

namespace syst
{

    class TcpConnection;

    class TcpStreambuf : public std::streambuf
    {
    public:
        explicit TcpStreambuf(TcpConnection &connection);
        TcpStreambuf(const TcpStreambuf &) = delete;
        ~TcpStreambuf();
        TcpStreambuf &operator=(const TcpStreambuf &) = delete;

    private:
        TcpConnection &connection;
        std::array<char, 4096> readBuf;
        std::array<uint8_t, 1024> writeBuf;

        std::streambuf::int_type overflow(std::streambuf::int_type ch) override;
        int sync() override;
        std::streambuf::int_type underflow() override;
        bool dumpBuffer(bool finalize);
    };

}
