// Copyright (c) 2012-2017, The MevaCoin developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero Project
// Copyright (c) 2018-2019, The TurtleCoin Developers
// Copyright (c) 2019, The Kryptokrona Developers
//
// Please see the included LICENSE file for more information.

#pragma once

#include <cstdint>
#include <cstddef>

namespace common
{

    class IOutputStream
    {
    public:
        virtual ~IOutputStream() {}
        virtual uint64_t writeSome(const void *data, uint64_t size) = 0;
    };

}
