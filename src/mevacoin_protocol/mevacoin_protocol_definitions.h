// Copyright (c) 2012-2017, The CryptoNote developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero Project
// Copyright (c) 2018-2019, The TurtleCoin Developers
// Copyright (c) 2019, The Kryptokrona Developers
//
// Please see the included LICENSE file for more information.

#pragma once

#include <list>
#include "mevacoin_core/mevacoin_basic.h"

// ISerializer-based serialization
#include "serialization/iserializer.h"
#include "serialization/serialization_overloads.h"
#include "mevacoin_core/mevacoin_serialization.h"

namespace mevacoin
{

#define BC_COMMANDS_POOL_BASE 2000

    /************************************************************************/
    /*                                                                      */
    /************************************************************************/

    // just to keep backward compatibility with BlockCompleteEntry serialization
    struct RawBlockLegacy
    {
        BinaryArray blockTemplate;
        std::vector<BinaryArray> transactions;
    };

    struct NOTIFY_NEW_BLOCK_request
    {
        RawBlockLegacy block;
        uint32_t current_blockchain_height;
        uint32_t hop;
    };

    struct NOTIFY_NEW_BLOCK
    {
        const static int ID = BC_COMMANDS_POOL_BASE + 1;
        typedef NOTIFY_NEW_BLOCK_request request;
    };

    /************************************************************************/
    /*                                                                      */
    /************************************************************************/
    struct NOTIFY_NEW_TRANSACTIONS_request
    {
        std::vector<BinaryArray> txs;
    };

    struct NOTIFY_NEW_TRANSACTIONS
    {
        const static int ID = BC_COMMANDS_POOL_BASE + 2;
        typedef NOTIFY_NEW_TRANSACTIONS_request request;
    };

    /************************************************************************/
    /*                                                                      */
    /************************************************************************/
    struct NOTIFY_REQUEST_GET_OBJECTS_request
    {
        std::vector<crypto::Hash> txs;
        std::vector<crypto::Hash> blocks;

        void serialize(ISerializer &s)
        {
            serializeAsBinary(txs, "txs", s);
            serializeAsBinary(blocks, "blocks", s);
        }
    };

    struct NOTIFY_REQUEST_GET_OBJECTS
    {
        const static int ID = BC_COMMANDS_POOL_BASE + 3;
        typedef NOTIFY_REQUEST_GET_OBJECTS_request request;
    };

    struct NOTIFY_RESPONSE_GET_OBJECTS_request
    {
        std::vector<std::string> txs;
        std::vector<RawBlockLegacy> blocks;
        std::vector<crypto::Hash> missed_ids;
        uint32_t current_blockchain_height;
    };

    struct NOTIFY_RESPONSE_GET_OBJECTS
    {
        const static int ID = BC_COMMANDS_POOL_BASE + 4;
        typedef NOTIFY_RESPONSE_GET_OBJECTS_request request;
    };

    struct NOTIFY_REQUEST_CHAIN
    {
        const static int ID = BC_COMMANDS_POOL_BASE + 6;

        struct request
        {
            std::vector<crypto::Hash> block_ids; /*IDs of the first 10 blocks are sequential, next goes with pow(2,n) offset, like 2, 4, 8, 16, 32, 64 and so on, and the last one is always genesis block */

            void serialize(ISerializer &s)
            {
                serializeAsBinary(block_ids, "block_ids", s);
            }
        };
    };

    struct NOTIFY_RESPONSE_CHAIN_ENTRY_request
    {
        uint32_t start_height;
        uint32_t total_height;
        std::vector<crypto::Hash> m_block_ids;

        void serialize(ISerializer &s)
        {
            KV_MEMBER(start_height)
            KV_MEMBER(total_height)
            serializeAsBinary(m_block_ids, "m_block_ids", s);
        }
    };

    struct NOTIFY_RESPONSE_CHAIN_ENTRY
    {
        const static int ID = BC_COMMANDS_POOL_BASE + 7;
        typedef NOTIFY_RESPONSE_CHAIN_ENTRY_request request;
    };

    /************************************************************************/
    /*                                                                      */
    /************************************************************************/
    struct NOTIFY_REQUEST_TX_POOL_request
    {
        std::vector<crypto::Hash> txs;

        void serialize(ISerializer &s)
        {
            serializeAsBinary(txs, "txs", s);
        }
    };

    struct NOTIFY_REQUEST_TX_POOL
    {
        const static int ID = BC_COMMANDS_POOL_BASE + 8;
        typedef NOTIFY_REQUEST_TX_POOL_request request;
    };

    /************************************************************************/
    /*                                                                      */
    /************************************************************************/
    struct NOTIFY_NEW_LITE_BLOCK_request
    {
        BinaryArray blockTemplate;
        uint32_t current_blockchain_height;
        uint32_t hop;
    };

    struct NOTIFY_NEW_LITE_BLOCK
    {
        const static int ID = BC_COMMANDS_POOL_BASE + 9;
        typedef NOTIFY_NEW_LITE_BLOCK_request request;
    };

    struct NOTIFY_MISSING_TXS_request
    {
        crypto::Hash blockHash;
        uint32_t current_blockchain_height;
        std::vector<crypto::Hash> missing_txs;
    };

    struct NOTIFY_MISSING_TXS
    {
        const static int ID = BC_COMMANDS_POOL_BASE + 10;
        typedef NOTIFY_MISSING_TXS_request request;
    };
}
