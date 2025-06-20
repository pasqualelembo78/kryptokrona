// Copyright (c) 2012-2017, The MevaCoin developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero Project
// Copyright (c) 2018-2019, The TurtleCoin Developers
// Copyright (c) 2019, The Kryptokrona Developers
//
// Please see the included LICENSE file for more information.

#include "std_output_stream.h"

namespace common
{

    StdOutputStream::StdOutputStream(std::ostream &out) : out(out)
    {
    }

    uint64_t StdOutputStream::writeSome(const void *data, uint64_t size)
    {
        out.write(static_cast<const char *>(data), size);
        if (out.bad())
        {
            return 0;
        }

        return size;
    }

}
