// Copyright (c) 2012-2017, The MevaCoin developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero Project
// Copyright (c) 2018-2019, The TurtleCoin Developers
// Copyright (c) 2019, The Kryptokrona Developers
//
// Please see the included LICENSE file for more information.

#pragma once

#include <string.h>
#include <tuple>
#include <common/string_tools.h>

struct NetworkAddress
{
    uint32_t ip;
    uint32_t port;
};

struct PeerlistEntry
{
    NetworkAddress adr;
    uint64_t id;
    uint64_t last_seen;
};

struct connection_entry
{
    NetworkAddress adr;
    uint64_t id;
    bool is_income;
};

inline bool operator<(const NetworkAddress &a, const NetworkAddress &b)
{
    return std::tie(a.ip, a.port) < std::tie(b.ip, b.port);
}

inline bool operator==(const NetworkAddress &a, const NetworkAddress &b)
{
    return memcmp(&a, &b, sizeof(a)) == 0;
}

inline std::ostream &operator<<(std::ostream &s, const NetworkAddress &na)
{
    return s << common::ipAddressToString(na.ip) << ":" << std::to_string(na.port);
}

inline uint32_t hostToNetwork(uint32_t n)
{
    return (n << 24) | (n & 0xff00) << 8 | (n & 0xff0000) >> 8 | (n >> 24);
}

inline uint32_t networkToHost(uint32_t n)
{
    return hostToNetwork(n); // the same
}
