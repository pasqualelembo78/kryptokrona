// Copyright (c) 2012-2017, The MevaCoin developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero Project
// Copyright (c) 2018-2019, The TurtleCoin Developers
// Copyright (c) 2019, The Kryptokrona Developers
//
// Please see the included LICENSE file for more information.

#pragma once

#include <string>
#include <system_error>

namespace tools
{
    std::string getDefaultDataDirectory();
    std::string get_os_version_string();
    bool create_directories_if_necessary(const std::string &path);
    bool directoryExists(const std::string &path);
}
