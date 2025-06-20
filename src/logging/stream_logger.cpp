// Copyright (c) 2012-2017, The MevaCoin developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero Project
// Copyright (c) 2018-2019, The TurtleCoin Developers
// Copyright (c) 2019, The Kryptokrona Developers
//
// Please see the included LICENSE file for more information.

#include "stream_logger.h"
#include <iostream>
#include <sstream>

namespace logging
{

    StreamLogger::StreamLogger(Level level) : CommonLogger(level), stream(nullptr)
    {
    }

    StreamLogger::StreamLogger(std::ostream &stream, Level level) : CommonLogger(level), stream(&stream)
    {
    }

    void StreamLogger::attachToStream(std::ostream &stream)
    {
        this->stream = &stream;
    }

    void StreamLogger::doLogString(const std::string &message)
    {
        if (stream != nullptr && stream->good())
        {
            std::lock_guard<std::mutex> lock(mutex);
            bool readingText = true;
            for (size_t charPos = 0; charPos < message.size(); ++charPos)
            {
                if (message[charPos] == ILogger::COLOR_DELIMETER)
                {
                    readingText = !readingText;
                }
                else if (readingText)
                {
                    *stream << message[charPos];
                }
            }

            *stream << std::flush;
        }
    }

}
