// Copyright (c) 2012-2017, The CryptoNote developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero Project
// Copyright (c) 2018-2019, The TurtleCoin Developers
// Copyright (c) 2019, The Kryptokrona Developers
//
// Please see the included LICENSE file for more information.

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "common/int_util.h"
#include "crypto/hash.h"
#include <config/mevacoin_config.h>
#include "check_difficulty.h"

namespace mevacoin
{

    using std::uint64_t;
    using std::vector;

#if defined(__SIZEOF_INT128__)

    static inline void mul(uint64_t a, uint64_t b, uint64_t &low, uint64_t &high)
    {
        typedef unsigned __int128 uint128_t;
        uint128_t res = (uint128_t)a * (uint128_t)b;
        low = (uint64_t)res;
        high = (uint64_t)(res >> 64);
    }

#else

    static inline void mul(uint64_t a, uint64_t b, uint64_t &low, uint64_t &high)
    {
        low = mul128(a, b, &high);
    }

#endif

    static inline bool cadd(uint64_t a, uint64_t b)
    {
        return a + b < a;
    }

    static inline bool cadc(uint64_t a, uint64_t b, bool c)
    {
        return a + b < a || (c && a + b == (uint64_t)-1);
    }

    bool check_hash(const crypto::Hash &hash, uint64_t difficulty)
    {
        uint64_t low, high, top, cur;
        // First check the highest word, this will most likely fail for a random hash.
        mul(swap64le(((const uint64_t *)&hash)[3]), difficulty, top, high);
        if (high != 0)
        {
            return false;
        }
        mul(swap64le(((const uint64_t *)&hash)[0]), difficulty, low, cur);
        mul(swap64le(((const uint64_t *)&hash)[1]), difficulty, low, high);
        bool carry = cadd(cur, low);
        cur = high;
        mul(swap64le(((const uint64_t *)&hash)[2]), difficulty, low, high);
        carry = cadc(cur, low, carry);
        carry = cadc(high, top, carry);
        return !carry;
    }
}
