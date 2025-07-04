// Copyright (c) 2012-2017, The CryptoNote developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero Project
// Copyright (c) 2018-2019, The TurtleCoin Developers
// Copyright (c) 2019, The Kryptokrona Developers
//
// Please see the included LICENSE file for more information.

#include "itransaction.h"

#include <numeric>
#include <system_error>
#include <memory>

#include "mevacoin_core/mevacoin_basic.h"
#include "mevacoin_core/transaction_api_extra.h"
#include "transaction_utils.h"
#include "mevacoin_core/mevacoin_tools.h"

using namespace crypto;

namespace mevacoin
{

    class TransactionPrefixImpl : public ITransactionReader
    {
    public:
        TransactionPrefixImpl();
        TransactionPrefixImpl(const TransactionPrefix &prefix, const Hash &transactionHash);

        virtual ~TransactionPrefixImpl() {}

        virtual Hash getTransactionHash() const override;
        virtual Hash getTransactionPrefixHash() const override;
        virtual PublicKey getTransactionPublicKey() const override;
        virtual uint64_t getUnlockTime() const override;

        // extra
        virtual bool getPaymentId(Hash &paymentId) const override;
        virtual bool getExtraNonce(BinaryArray &nonce) const override;
        virtual BinaryArray getExtra() const override;

        // inputs
        virtual size_t getInputCount() const override;
        virtual uint64_t getInputTotalAmount() const override;
        virtual TransactionTypes::InputType getInputType(size_t index) const override;
        virtual void getInput(size_t index, KeyInput &input) const override;

        // outputs
        virtual size_t getOutputCount() const override;
        virtual uint64_t getOutputTotalAmount() const override;
        virtual TransactionTypes::OutputType getOutputType(size_t index) const override;
        virtual void getOutput(size_t index, KeyOutput &output, uint64_t &amount) const override;

        // signatures
        virtual size_t getRequiredSignaturesCount(size_t inputIndex) const override;
        virtual bool findOutputsToAccount(const AccountPublicAddress &addr, const SecretKey &viewSecretKey, std::vector<uint32_t> &outs, uint64_t &outputAmount) const override;

        // serialized transaction
        virtual BinaryArray getTransactionData() const override;

    private:
        TransactionPrefix m_txPrefix;
        TransactionExtra m_extra;
        Hash m_txHash;
    };

    TransactionPrefixImpl::TransactionPrefixImpl()
    {
    }

    TransactionPrefixImpl::TransactionPrefixImpl(const TransactionPrefix &prefix, const Hash &transactionHash)
    {
        m_extra.parse(prefix.extra);

        m_txPrefix = prefix;
        m_txHash = transactionHash;
    }

    Hash TransactionPrefixImpl::getTransactionHash() const
    {
        return m_txHash;
    }

    Hash TransactionPrefixImpl::getTransactionPrefixHash() const
    {
        return getObjectHash(m_txPrefix);
    }

    PublicKey TransactionPrefixImpl::getTransactionPublicKey() const
    {
        crypto::PublicKey pk(NULL_PUBLIC_KEY);
        m_extra.getPublicKey(pk);
        return pk;
    }

    uint64_t TransactionPrefixImpl::getUnlockTime() const
    {
        return m_txPrefix.unlockTime;
    }

    bool TransactionPrefixImpl::getPaymentId(Hash &hash) const
    {
        BinaryArray nonce;

        if (getExtraNonce(nonce))
        {
            crypto::Hash paymentId;
            if (getPaymentIdFromTransactionExtraNonce(nonce, paymentId))
            {
                hash = reinterpret_cast<const Hash &>(paymentId);
                return true;
            }
        }

        return false;
    }

    bool TransactionPrefixImpl::getExtraNonce(BinaryArray &nonce) const
    {
        TransactionExtraNonce extraNonce;

        if (m_extra.get(extraNonce))
        {
            nonce = extraNonce.nonce;
            return true;
        }

        return false;
    }

    BinaryArray TransactionPrefixImpl::getExtra() const
    {
        return m_txPrefix.extra;
    }

    size_t TransactionPrefixImpl::getInputCount() const
    {
        return m_txPrefix.inputs.size();
    }

    uint64_t TransactionPrefixImpl::getInputTotalAmount() const
    {
        return std::accumulate(m_txPrefix.inputs.begin(), m_txPrefix.inputs.end(), 0ULL, [](uint64_t val, const TransactionInput &in)
                               { return val + getTransactionInputAmount(in); });
    }

    TransactionTypes::InputType TransactionPrefixImpl::getInputType(size_t index) const
    {
        return getTransactionInputType(getInputChecked(m_txPrefix, index));
    }

    void TransactionPrefixImpl::getInput(size_t index, KeyInput &input) const
    {
        input = boost::get<KeyInput>(getInputChecked(m_txPrefix, index, TransactionTypes::InputType::Key));
    }

    size_t TransactionPrefixImpl::getOutputCount() const
    {
        return m_txPrefix.outputs.size();
    }

    uint64_t TransactionPrefixImpl::getOutputTotalAmount() const
    {
        return std::accumulate(m_txPrefix.outputs.begin(), m_txPrefix.outputs.end(), 0ULL, [](uint64_t val, const TransactionOutput &out)
                               { return val + out.amount; });
    }

    TransactionTypes::OutputType TransactionPrefixImpl::getOutputType(size_t index) const
    {
        return getTransactionOutputType(getOutputChecked(m_txPrefix, index).target);
    }

    void TransactionPrefixImpl::getOutput(size_t index, KeyOutput &output, uint64_t &amount) const
    {
        const auto &out = getOutputChecked(m_txPrefix, index, TransactionTypes::OutputType::Key);
        output = boost::get<KeyOutput>(out.target);
        amount = out.amount;
    }

    size_t TransactionPrefixImpl::getRequiredSignaturesCount(size_t inputIndex) const
    {
        return ::mevacoin::getRequiredSignaturesCount(getInputChecked(m_txPrefix, inputIndex));
    }

    bool TransactionPrefixImpl::findOutputsToAccount(const AccountPublicAddress &addr, const SecretKey &viewSecretKey, std::vector<uint32_t> &outs, uint64_t &outputAmount) const
    {
        return ::mevacoin::findOutputsToAccount(m_txPrefix, addr, viewSecretKey, outs, outputAmount);
    }

    BinaryArray TransactionPrefixImpl::getTransactionData() const
    {
        return toBinaryArray(m_txPrefix);
    }

    std::unique_ptr<ITransactionReader> createTransactionPrefix(const TransactionPrefix &prefix, const Hash &transactionHash)
    {
        return std::unique_ptr<ITransactionReader>(new TransactionPrefixImpl(prefix, transactionHash));
    }

    std::unique_ptr<ITransactionReader> createTransactionPrefix(const Transaction &fullTransaction)
    {
        return std::unique_ptr<ITransactionReader>(new TransactionPrefixImpl(fullTransaction, getObjectHash(fullTransaction)));
    }

}
