// Copyright (c) 2012-2017, The CryptoNote developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero Project
// Copyright (c) 2018-2019, The TurtleCoin Developers
// Copyright (c) 2019, The Kryptokrona Developers
//
// Please see the included LICENSE file for more information.

#pragma once

#include <limits>
#include "common/memory_input_stream.h"
#include "common/string_tools.h"
#include "common/vector_output_stream.h"
#include "serialization/binary_output_stream_serializer.h"
#include "serialization/binary_input_stream_serializer.h"
#include <config/mevacoin_config.h>
#include "mevacoin_serialization.h"

namespace mevacoin
{

    void getBinaryArrayHash(const BinaryArray &binaryArray, crypto::Hash &hash);
    crypto::Hash getBinaryArrayHash(const BinaryArray &binaryArray);

    // noexcept
    template <class T>
    bool toBinaryArray(const T &object, BinaryArray &binaryArray)
    {
        try
        {
            binaryArray = toBinaryArray(object);
        }
        catch (std::exception &)
        {
            return false;
        }

        return true;
    }

    template <>
    bool toBinaryArray(const BinaryArray &object, BinaryArray &binaryArray);

    // throws exception if serialization failed
    template <class T>
    BinaryArray toBinaryArray(const T &object)
    {
        BinaryArray ba;
        ::common::VectorOutputStream stream(ba);
        BinaryOutputStreamSerializer serializer(stream);
        serialize(const_cast<T &>(object), serializer);
        return ba;
    }

    template <class T>
    T fromBinaryArray(const BinaryArray &binaryArray)
    {
        T object;
        common::MemoryInputStream stream(binaryArray.data(), binaryArray.size());
        BinaryInputStreamSerializer serializer(stream);
        serialize(object, serializer);
        if (!stream.endOfStream())
        { // check that all data was consumed
            throw std::runtime_error("failed to unpack type");
        }

        return object;
    }

    template <class T>
    bool fromBinaryArray(T &object, const BinaryArray &binaryArray)
    {
        try
        {
            object = fromBinaryArray<T>(binaryArray);
        }
        catch (std::exception &)
        {
            return false;
        }

        return true;
    }

    template <class T>
    bool getObjectBinarySize(const T &object, size_t &size)
    {
        BinaryArray ba;
        if (!toBinaryArray(object, ba))
        {
            size = (std::numeric_limits<size_t>::max)();
            return false;
        }

        size = ba.size();
        return true;
    }

    template <class T>
    size_t getObjectBinarySize(const T &object)
    {
        size_t size;
        getObjectBinarySize(object, size);
        return size;
    }

    template <class T>
    bool getObjectHash(const T &object, crypto::Hash &hash)
    {
        BinaryArray ba;
        if (!toBinaryArray(object, ba))
        {
            hash = NULL_HASH;
            return false;
        }

        hash = getBinaryArrayHash(ba);
        return true;
    }

    template <class T>
    bool getObjectHash(const T &object, crypto::Hash &hash, size_t &size)
    {
        BinaryArray ba;
        if (!toBinaryArray(object, ba))
        {
            hash = NULL_HASH;
            size = (std::numeric_limits<size_t>::max)();
            return false;
        }

        size = ba.size();
        hash = getBinaryArrayHash(ba);
        return true;
    }

    template <class T>
    crypto::Hash getObjectHash(const T &object)
    {
        crypto::Hash hash;
        getObjectHash(object, hash);
        return hash;
    }

    inline bool getBaseTransactionHash(const BaseTransaction &tx, crypto::Hash &hash)
    {
        if (tx.version < TRANSACTION_VERSION_2)
        {
            return getObjectHash(tx, hash);
        }
        else
        {
            BinaryArray data{{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xbc, 0x36, 0x78, 0x9e, 0x7a, 0x1e, 0x28, 0x14, 0x36, 0x46, 0x42, 0x29, 0x82, 0x8f, 0x81, 0x7d, 0x66, 0x12, 0xf7, 0xb4, 0x77, 0xd6, 0x65, 0x91, 0xff, 0x96, 0xa9, 0xe0, 0x64, 0xbc, 0xc9, 0x8a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
            if (getObjectHash(static_cast<const TransactionPrefix &>(tx), *reinterpret_cast<crypto::Hash *>(data.data())))
            {
                hash = getBinaryArrayHash(data);
                return true;
            }
            else
            {
                return false;
            }
        }
    }

    uint64_t getInputAmount(const Transaction &transaction);
    std::vector<uint64_t> getInputsAmounts(const Transaction &transaction);
    uint64_t getOutputAmount(const Transaction &transaction);
    void decomposeAmount(uint64_t amount, uint64_t dustThreshold, std::vector<uint64_t> &decomposedAmounts);
}
