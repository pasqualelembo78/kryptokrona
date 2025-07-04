// Copyright (c) 2012-2017, The CryptoNote developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero Project
// Copyright (c) 2018-2019, The TurtleCoin Developers
// Copyright (c) 2019, The Kryptokrona Developers
//
// Please see the included LICENSE file for more information.

#pragma once

#include "itransaction_pool_cleaner.h"

#include <chrono>
#include <unordered_map>

#include "crypto/crypto.h"

#include "mevacoin_core/itime_provider.h"
#include "itransaction_pool.h"
#include "logging/ilogger.h"
#include "logging/logger_ref.h"

#include <syst/context_group.h>

namespace mevacoin
{

    using json = nlohmann::json;

    class TransactionPoolCleanWrapper : public ITransactionPoolCleanWrapper
    {
    public:
        TransactionPoolCleanWrapper(
            std::unique_ptr<ITransactionPool> &&transactionPool,
            std::unique_ptr<ITimeProvider> &&timeProvider,
            std::shared_ptr<logging::ILogger> logger,
            uint64_t timeout);

        TransactionPoolCleanWrapper(const TransactionPoolCleanWrapper &) = delete;
        TransactionPoolCleanWrapper(TransactionPoolCleanWrapper &&other) = delete;

        TransactionPoolCleanWrapper &operator=(const TransactionPoolCleanWrapper &) = delete;
        TransactionPoolCleanWrapper &operator=(TransactionPoolCleanWrapper &&) = delete;

        virtual ~TransactionPoolCleanWrapper();

        virtual bool pushTransaction(CachedTransaction &&tx, TransactionValidatorState &&transactionState) override;
        virtual const CachedTransaction &getTransaction(const crypto::Hash &hash) const override;
        virtual bool removeTransaction(const crypto::Hash &hash) override;

        virtual size_t getTransactionCount() const override;
        virtual std::vector<crypto::Hash> getTransactionHashes() const override;
        virtual bool checkIfTransactionPresent(const crypto::Hash &hash) const override;

        virtual const TransactionValidatorState &getPoolTransactionValidationState() const override;
        virtual std::vector<CachedTransaction> getPoolTransactions() const override;

        virtual uint64_t getTransactionReceiveTime(const crypto::Hash &hash) const override;
        virtual std::vector<crypto::Hash> getTransactionHashesByPaymentId(const crypto::Hash &paymentId) const override;

        std::string hex2ascii(const std::string &hex);
        json trimExtra(const std::string &extra);
        virtual std::vector<crypto::Hash> clean(const uint32_t height) override;

        virtual void flush() override;

    private:
        std::unique_ptr<ITransactionPool> transactionPool;
        std::unique_ptr<ITimeProvider> timeProvider;
        logging::LoggerRef logger;
        std::unordered_map<crypto::Hash, uint64_t> recentlyDeletedTransactions;
        uint64_t timeout;

        bool isTransactionRecentlyDeleted(const crypto::Hash &hash) const;
        void cleanRecentlyDeletedTransactions(uint64_t currentTime);
    };

} // namespace mevacoin
