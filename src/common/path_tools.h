// Copyright (c) 2012-2017, The MevaCoin developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero Project
// Copyright (c) 2018-2019, The TurtleCoin Developers
// Copyright (c) 2019, The Kryptokrona Developers
//
// Please see the included LICENSE file for more information.

#pragma once

#include <string>

namespace common
{

    std::string NativePathToGeneric(const std::string &nativePath);

    std::string GetPathDirectory(const std::string &path);

    std::string CombinePath(const std::string &path1, const std::string &path2);
    std::string RemoveExtension(const std::string &path);
    std::string ReplaceExtenstion(const std::string &path, const std::string &extension);
    bool HasParentPath(const std::string &path);

}
