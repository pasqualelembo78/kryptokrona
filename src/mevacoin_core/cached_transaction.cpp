// Copyright (c) 2012-2017, The CryptoNote developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero Project
// Copyright (c) 2018-2019, The TurtleCoin Developers
// Copyright (c) 2019, The Kryptokrona Developers
//
// Please see the included LICENSE file for more information.

#include "cached_transaction.h"
#include <common/varint.h>
#include <config/mevacoin_config.h>
#include "mevacoin_tools.h"

using namespace crypto;
using namespace mevacoin;

CachedTransaction::CachedTransaction(Transaction &&transaction) : transaction(std::move(transaction))
{
}

CachedTransaction::CachedTransaction(const Transaction &transaction) : transaction(transaction)
{
}

CachedTransaction::CachedTransaction(const BinaryArray &transactionBinaryArray) : transactionBinaryArray(transactionBinaryArray)
{
    if (!fromBinaryArray<Transaction>(transaction, this->transactionBinaryArray.get()))
    {
        throw std::runtime_error("CachedTransaction::CachedTransaction(BinaryArray&&), deserealization error.");
    }
}

const Transaction &CachedTransaction::getTransaction() const
{
    return transaction;
}

const crypto::Hash &CachedTransaction::getTransactionHash() const
{
    if (!transactionHash.is_initialized())
    {
        transactionHash = getBinaryArrayHash(getTransactionBinaryArray());
    }

    return transactionHash.get();
}

const crypto::Hash &CachedTransaction::getTransactionPrefixHash() const
{
    if (!transactionPrefixHash.is_initialized())
    {
        transactionPrefixHash = getObjectHash(static_cast<const TransactionPrefix &>(transaction));
    }

    return transactionPrefixHash.get();
}

const BinaryArray &CachedTransaction::getTransactionBinaryArray() const
{
    if (!transactionBinaryArray.is_initialized())
    {
        transactionBinaryArray = toBinaryArray(transaction);
    }

    return transactionBinaryArray.get();
}

uint64_t CachedTransaction::getTransactionFee() const
{
    if (!transactionFee.is_initialized())
    {
        uint64_t summaryInputAmount = 0;
        uint64_t summaryOutputAmount = 0;
        for (auto &out : transaction.outputs)
        {
            summaryOutputAmount += out.amount;
        }

        for (auto &in : transaction.inputs)
        {
            if (in.type() == typeid(KeyInput))
            {
                summaryInputAmount += boost::get<KeyInput>(in).amount;
            }
            else if (in.type() == typeid(BaseInput))
            {
                return 0;
            }
            else
            {
                assert(false && "Unknown out type");
            }
        }

        transactionFee = summaryInputAmount - summaryOutputAmount;
    }

    return transactionFee.get();
}
