// Copyright (c) 2012-2017, The MevaCoin developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero Project
// Copyright (c) 2018-2019, The TurtleCoin Developers
// Copyright (c) 2019, The Kryptokrona Developers
//
// Please see the included LICENSE file for more information.

#pragma once

#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace common
{

    std::string asString(const void *data, uint64_t size);  // Does not throw
    std::string asString(const std::vector<uint8_t> &data); // Does not throw
    std::vector<uint8_t> asBinaryArray(const std::string &data);

    uint8_t fromHex(char character);                                                        // Returns value of hex 'character', throws on error
    bool fromHex(char character, uint8_t &value);                                           // Assigns value of hex 'character' to 'value', returns false on error, does not throw
    uint64_t fromHex(const std::string &text, void *data, uint64_t bufferSize);             // Assigns values of hex 'text' to buffer 'data' up to 'bufferSize', returns actual data size, throws on error
    bool fromHex(const std::string &text, void *data, uint64_t bufferSize, uint64_t &size); // Assigns values of hex 'text' to buffer 'data' up to 'bufferSize', assigns actual data size to 'size', returns false on error, does not throw
    std::vector<uint8_t> fromHex(const std::string &text);                                  // Returns values of hex 'text', throws on error
    bool fromHex(const std::string &text, std::vector<uint8_t> &data);                      // Appends values of hex 'text' to 'data', returns false on error, does not throw

    template <typename T>
    bool podFromHex(const std::string &text, T &val)
    {
        uint64_t outSize;
        return fromHex(text, &val, sizeof(val), outSize) && outSize == sizeof(val);
    }

    std::string toHex(const void *data, uint64_t size);              // Returns hex representation of ('data', 'size'), does not throw
    void toHex(const void *data, uint64_t size, std::string &text);  // Appends hex representation of ('data', 'size') to 'text', does not throw
    std::string toHex(const std::vector<uint8_t> &data);             // Returns hex representation of 'data', does not throw
    void toHex(const std::vector<uint8_t> &data, std::string &text); // Appends hex representation of 'data' to 'text', does not throw

    template <class T>
    std::string podToHex(const T &s)
    {
        return toHex(&s, sizeof(s));
    }

    std::string extract(std::string &text, char delimiter);                         // Does not throw
    std::string extract(const std::string &text, char delimiter, uint64_t &offset); // Does not throw

    template <typename T>
    T fromString(const std::string &text)
    { // Throws on error
        T value;
        std::istringstream stream(text);
        stream >> value;
        if (stream.fail())
        {
            throw std::runtime_error("fromString: unable to parse value");
        }

        return value;
    }

    template <typename T>
    bool fromString(const std::string &text, T &value)
    { // Does not throw
        std::istringstream stream(text);
        stream >> value;
        return !stream.fail();
    }

    template <typename T>
    std::vector<T> fromDelimitedString(const std::string &source, char delimiter)
    { // Throws on error
        std::vector<T> data;
        for (uint64_t offset = 0; offset != source.size();)
        {
            data.emplace_back(fromString<T>(extract(source, delimiter, offset)));
        }

        return data;
    }

    template <typename T>
    bool fromDelimitedString(const std::string &source, char delimiter, std::vector<T> &data)
    { // Does not throw
        for (uint64_t offset = 0; offset != source.size();)
        {
            T value;
            if (!fromString<T>(extract(source, delimiter, offset), value))
            {
                return false;
            }

            data.emplace_back(value);
        }

        return true;
    }

    template <typename T>
    std::string toString(const T &value)
    { // Does not throw
        std::ostringstream stream;
        stream << value;
        return stream.str();
    }

    template <typename T>
    void toString(const T &value, std::string &text)
    { // Does not throw
        std::ostringstream stream;
        stream << value;
        text += stream.str();
    }

    bool saveStringToFile(const std::string &filepath, const std::string &buf);

    std::string ipAddressToString(uint32_t ip);
    bool parseIpAddressAndPort(uint32_t &ip, uint32_t &port, const std::string &addr);

    std::string timeIntervalToString(uint64_t intervalInSeconds);

    void trim(std::string &str);

    void leftTrim(std::string &str);

    void rightTrim(std::string &str);

}
