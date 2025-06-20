// Copyright (c) 2012-2017, The MevaCoin developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero Project
// Copyright (c) 2018-2019, The TurtleCoin Developers
// Copyright (c) 2019, The Kryptokrona Developers
//
// Please see the included LICENSE file for more information.

#pragma once

#include <iostream>
#include "ilogger.h"

namespace logging
{

    class LoggerMessage : public std::ostream, std::streambuf
    {
    public:
        LoggerMessage(std::shared_ptr<ILogger> logger, const std::string &category, Level level, const std::string &color);
        ~LoggerMessage();
        LoggerMessage(const LoggerMessage &) = delete;
        LoggerMessage &operator=(const LoggerMessage &) = delete;
        LoggerMessage(LoggerMessage &&other);

    private:
        int sync() override;
        std::streamsize xsputn(const char *s, std::streamsize n) override;
        int overflow(int c) override;

        std::string message;
        const std::string category;
        Level logLevel;
        std::shared_ptr<ILogger> logger;
        boost::posix_time::ptime timestamp;
        bool gotText;
    };

}
