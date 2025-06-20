// Copyright (c) 2012-2017, The MevaCoin developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero Project
// Copyright (c) 2018-2019, The TurtleCoin Developers
// Copyright (c) 2019, The Kryptokrona Developers
//
// Please see the included LICENSE file for more information.

#pragma once

#include <fstream>
#include "stream_logger.h"

namespace logging
{

    class FileLogger : public StreamLogger
    {
    public:
        FileLogger(Level level = DEBUGGING);
        void init(const std::string &filename);

    private:
        std::ofstream fileStream;
    };

}
