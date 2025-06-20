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

    class Ipv4Address
    {
    public:
        explicit Ipv4Address(uint32_t value);
        explicit Ipv4Address(const std::string &dottedDecimal);
        bool operator!=(const Ipv4Address &other) const;
        bool operator==(const Ipv4Address &other) const;
        uint32_t getValue() const;
        bool isLoopback() const;
        bool isPrivate() const;
        std::string toDottedDecimal() const;

    private:
        uint32_t value;
    };

}
