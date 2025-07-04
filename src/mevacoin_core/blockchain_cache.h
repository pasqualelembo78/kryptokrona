// Copyright (c) 2012-2017, The CryptoNote developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero Project
// Copyright (c) 2018-2019, The TurtleCoin Developers
// Copyright (c) 2019, The Kryptokrona Developers
//
// Please see the included LICENSE file for more information.

#pragma once
#include <map>
#include <unordered_map>
#include <vector>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/random_access_index.hpp>

#include "blockchain_storage.h"
#include "common/string_view.h"
#include "currency.h"
#include "iblockchain_cache.h"
#include "mevacoin_core/upgrade_manager.h"

namespace mevacoin
{

    class ISerializer;

    struct SpentKeyImage
    {
        uint32_t blockIndex;
        crypto::KeyImage keyImage;

        void serialize(ISerializer &s);
    };

    struct CachedTransactionInfo
    {
        uint32_t blockIndex;
        uint32_t transactionIndex;
        crypto::Hash transactionHash;
        uint64_t unlockTime;
        std::vector<TransactionOutputTarget> outputs;
        // needed for getTransactionGlobalIndexes query
        std::vector<uint32_t> globalIndexes;

        void serialize(ISerializer &s);
    };

    struct CachedBlockInfo
    {
        crypto::Hash blockHash;
        uint64_t timestamp;
        uint64_t cumulativeDifficulty;
        uint64_t alreadyGeneratedCoins;
        uint64_t alreadyGeneratedTransactions;
        uint32_t blockSize;

        void serialize(ISerializer &s);
    };

    struct OutputGlobalIndexesForAmount
    {
        uint32_t startIndex = 0;

        // 1. This container must be sorted by PackedOutIndex::blockIndex and PackedOutIndex::transactionIndex
        // 2. GlobalOutputIndex for particular output is calculated as following: startIndex + index in vector
        std::vector<PackedOutIndex> outputs;

        void serialize(ISerializer &s);
    };

    struct PaymentIdTransactionHashPair
    {
        crypto::Hash paymentId;
        crypto::Hash transactionHash;

        void serialize(ISerializer &s);
    };

    bool serialize(PackedOutIndex &value, common::StringView name, mevacoin::ISerializer &serializer);

    class DatabaseBlockchainCache;

    class BlockchainCache : public IBlockchainCache
    {
    public:
        BlockchainCache(const std::string &filename, const Currency &currency, std::shared_ptr<logging::ILogger> logger, IBlockchainCache *parent, uint32_t startIndex = 0);

        // Returns upper part of segment. [this] remains lower part.
        // All of indexes on blockIndex == splitBlockIndex belong to upper part
        std::unique_ptr<IBlockchainCache> split(uint32_t splitBlockIndex) override;
        virtual void pushBlock(const CachedBlock &cachedBlock,
                               const std::vector<CachedTransaction> &cachedTransactions,
                               const TransactionValidatorState &validatorState,
                               size_t blockSize,
                               uint64_t generatedCoins,
                               uint64_t blockDifficulty,
                               RawBlock &&rawBlock) override;

        virtual PushedBlockInfo getPushedBlockInfo(uint32_t index) const override;
        bool checkIfSpent(const crypto::KeyImage &keyImage, uint32_t blockIndex) const override;
        bool checkIfSpent(const crypto::KeyImage &keyImage) const override;

        bool isTransactionSpendTimeUnlocked(uint64_t unlockTime) const override;
        bool isTransactionSpendTimeUnlocked(uint64_t unlockTime, uint32_t blockIndex) const override;

        ExtractOutputKeysResult extractKeyOutputKeys(uint64_t amount, common::ArrayView<uint32_t> globalIndexes, std::vector<crypto::PublicKey> &publicKeys) const override;
        ExtractOutputKeysResult extractKeyOutputKeys(uint64_t amount, uint32_t blockIndex, common::ArrayView<uint32_t> globalIndexes, std::vector<crypto::PublicKey> &publicKeys) const override;

        ExtractOutputKeysResult extractKeyOtputIndexes(uint64_t amount, common::ArrayView<uint32_t> globalIndexes, std::vector<PackedOutIndex> &outIndexes) const override;
        ExtractOutputKeysResult extractKeyOtputReferences(uint64_t amount, common::ArrayView<uint32_t> globalIndexes, std::vector<std::pair<crypto::Hash, size_t>> &outputReferences) const override;

        uint32_t getTopBlockIndex() const override;
        const crypto::Hash &getTopBlockHash() const override;
        uint32_t getBlockCount() const override;
        bool hasBlock(const crypto::Hash &blockHash) const override;
        uint32_t getBlockIndex(const crypto::Hash &blockHash) const override;

        bool hasTransaction(const crypto::Hash &transactionHash) const override;

        std::vector<uint64_t> getLastTimestamps(size_t count) const override;
        std::vector<uint64_t> getLastTimestamps(size_t count, uint32_t blockIndex, UseGenesis) const override;

        std::vector<uint64_t> getLastBlocksSizes(size_t count) const override;
        std::vector<uint64_t> getLastBlocksSizes(size_t count, uint32_t blockIndex, UseGenesis) const override;

        std::vector<uint64_t> getLastCumulativeDifficulties(size_t count, uint32_t blockIndex, UseGenesis) const override;
        std::vector<uint64_t> getLastCumulativeDifficulties(size_t count) const override;

        uint64_t getDifficultyForNextBlock() const override;
        uint64_t getDifficultyForNextBlock(uint32_t blockIndex) const override;

        virtual uint64_t getCurrentCumulativeDifficulty() const override;
        virtual uint64_t getCurrentCumulativeDifficulty(uint32_t blockIndex) const override;

        uint64_t getAlreadyGeneratedCoins() const override;
        uint64_t getAlreadyGeneratedCoins(uint32_t blockIndex) const override;
        uint64_t getAlreadyGeneratedTransactions(uint32_t blockIndex) const override;
        std::vector<uint64_t> getLastUnits(size_t count, uint32_t blockIndex, UseGenesis use,
                                           std::function<uint64_t(const CachedBlockInfo &)> pred) const override;

        crypto::Hash getBlockHash(uint32_t blockIndex) const override;
        virtual std::vector<crypto::Hash> getBlockHashes(uint32_t startIndex, size_t maxCount) const override;

        virtual IBlockchainCache *getParent() const override;
        virtual void setParent(IBlockchainCache *p) override;
        virtual uint32_t getStartBlockIndex() const override;

        virtual size_t getKeyOutputsCountForAmount(uint64_t amount, uint32_t blockIndex) const override;

        std::tuple<bool, uint64_t> getBlockHeightForTimestamp(uint64_t timestamp) const override;

        virtual uint32_t getTimestampLowerBoundBlockIndex(uint64_t timestamp) const override;
        virtual bool getTransactionGlobalIndexes(const crypto::Hash &transactionHash, std::vector<uint32_t> &globalIndexes) const override;
        virtual size_t getTransactionCount() const override;
        virtual uint32_t getBlockIndexContainingTx(const crypto::Hash &transactionHash) const override;

        virtual size_t getChildCount() const override;
        virtual void addChild(IBlockchainCache *child) override;
        virtual bool deleteChild(IBlockchainCache *) override;

        virtual void save() override;
        virtual void load() override;

        virtual std::vector<BinaryArray> getRawTransactions(const std::vector<crypto::Hash> &transactions,
                                                            std::vector<crypto::Hash> &missedTransactions) const override;
        virtual std::vector<BinaryArray> getRawTransactions(const std::vector<crypto::Hash> &transactions) const override;
        void getRawTransactions(const std::vector<crypto::Hash> &transactions,
                                std::vector<BinaryArray> &foundTransactions,
                                std::vector<crypto::Hash> &missedTransactions) const override;

        virtual std::unordered_map<crypto::Hash, std::vector<uint64_t>> getGlobalIndexes(
            const std::vector<crypto::Hash> transactionHashes) const override;

        virtual RawBlock getBlockByIndex(uint32_t index) const override;
        virtual BinaryArray getRawTransaction(uint32_t blockIndex, uint32_t transactionIndex) const override;
        virtual std::vector<crypto::Hash> getTransactionHashes() const override;
        virtual std::vector<uint32_t> getRandomOutsByAmount(uint64_t amount, size_t count, uint32_t blockIndex) const override;
        virtual ExtractOutputKeysResult extractKeyOutputs(uint64_t amount, uint32_t blockIndex, common::ArrayView<uint32_t> globalIndexes,
                                                          std::function<ExtractOutputKeysResult(const CachedTransactionInfo &info, PackedOutIndex index,
                                                                                                uint32_t globalIndex)>
                                                              pred) const override;

        virtual std::vector<crypto::Hash> getTransactionHashesByPaymentId(const crypto::Hash &paymentId) const override;
        virtual std::vector<crypto::Hash> getBlockHashesByTimestamps(uint64_t timestampBegin, size_t secondsCount) const override;

        virtual std::vector<RawBlock> getBlocksByHeight(
            const uint64_t startHeight,
            const uint64_t endHeight) const override;

    private:
        struct BlockIndexTag
        {
        };
        struct BlockHashTag
        {
        };
        struct TransactionHashTag
        {
        };
        struct KeyImageTag
        {
        };
        struct TransactionInBlockTag
        {
        };
        struct PackedOutputTag
        {
        };
        struct TimestampTag
        {
        };
        struct PaymentIdTag
        {
        };

        typedef boost::multi_index_container<
            SpentKeyImage,
            boost::multi_index::indexed_by<
                boost::multi_index::ordered_non_unique<
                    boost::multi_index::tag<BlockIndexTag>,
                    BOOST_MULTI_INDEX_MEMBER(SpentKeyImage, uint32_t, blockIndex)>,
                boost::multi_index::hashed_unique<
                    boost::multi_index::tag<KeyImageTag>,
                    BOOST_MULTI_INDEX_MEMBER(SpentKeyImage, crypto::KeyImage, keyImage)>>>
            SpentKeyImagesContainer;

        typedef boost::multi_index_container<
            CachedTransactionInfo,
            boost::multi_index::indexed_by<
                boost::multi_index::hashed_unique<
                    boost::multi_index::tag<TransactionInBlockTag>,
                    boost::multi_index::composite_key<
                        CachedTransactionInfo,
                        BOOST_MULTI_INDEX_MEMBER(CachedTransactionInfo, uint32_t, blockIndex),
                        BOOST_MULTI_INDEX_MEMBER(CachedTransactionInfo, uint32_t, transactionIndex)>>,
                boost::multi_index::ordered_non_unique<
                    boost::multi_index::tag<BlockIndexTag>,
                    BOOST_MULTI_INDEX_MEMBER(CachedTransactionInfo, uint32_t, blockIndex)>,
                boost::multi_index::hashed_unique<
                    boost::multi_index::tag<TransactionHashTag>,
                    BOOST_MULTI_INDEX_MEMBER(CachedTransactionInfo, crypto::Hash, transactionHash)>>>
            TransactionsCacheContainer;

        typedef boost::multi_index_container<
            CachedBlockInfo,
            boost::multi_index::indexed_by<
                // The index here is blockIndex - startIndex
                boost::multi_index::random_access<
                    boost::multi_index::tag<BlockIndexTag>>,
                boost::multi_index::hashed_unique<
                    boost::multi_index::tag<BlockHashTag>,
                    BOOST_MULTI_INDEX_MEMBER(CachedBlockInfo, crypto::Hash, blockHash)>,
                boost::multi_index::ordered_non_unique<
                    boost::multi_index::tag<TimestampTag>,
                    BOOST_MULTI_INDEX_MEMBER(CachedBlockInfo, uint64_t, timestamp)>>>
            BlockInfoContainer;

        typedef boost::multi_index_container<
            PaymentIdTransactionHashPair,
            boost::multi_index::indexed_by<
                boost::multi_index::hashed_non_unique<
                    boost::multi_index::tag<PaymentIdTag>,
                    BOOST_MULTI_INDEX_MEMBER(PaymentIdTransactionHashPair, crypto::Hash, paymentId)>,
                boost::multi_index::hashed_unique<
                    boost::multi_index::tag<TransactionHashTag>,
                    BOOST_MULTI_INDEX_MEMBER(PaymentIdTransactionHashPair, crypto::Hash, transactionHash)>>>
            PaymentIdContainer;

        typedef std::map<uint64_t, OutputGlobalIndexesForAmount> OutputsGlobalIndexesContainer;
        typedef std::map<BlockIndex, std::vector<std::pair<Amount, GlobalOutputIndex>>> OutputSpentInBlock;
        typedef std::set<std::pair<Amount, GlobalOutputIndex>> SpentOutputsOnAmount;

        const uint32_t CURRENT_SERIALIZATION_VERSION = 1;
        std::string filename;
        const Currency &currency;
        logging::LoggerRef logger;
        IBlockchainCache *parent;
        // index of first block stored in this cache
        uint32_t startIndex;

        TransactionsCacheContainer transactions;
        SpentKeyImagesContainer spentKeyImages;
        BlockInfoContainer blockInfos;
        OutputsGlobalIndexesContainer keyOutputsGlobalIndexes;
        PaymentIdContainer paymentIds;
        std::unique_ptr<BlockchainStorage> storage;

        std::vector<IBlockchainCache *> children;

        void serialize(ISerializer &s);

        void addSpentKeyImage(const crypto::KeyImage &keyImage, uint32_t blockIndex);
        void pushTransaction(const CachedTransaction &tx, uint32_t blockIndex, uint16_t transactionBlockIndex);

        void splitSpentKeyImages(BlockchainCache &newCache, uint32_t splitBlockIndex);
        void splitTransactions(BlockchainCache &newCache, uint32_t splitBlockIndex);
        void splitBlocks(BlockchainCache &newCache, uint32_t splitBlockIndex);
        void splitKeyOutputsGlobalIndexes(BlockchainCache &newCache, uint32_t splitBlockIndex);
        void removePaymentId(const crypto::Hash &transactionHash, BlockchainCache &newCache);

        uint32_t insertKeyOutputToGlobalIndex(uint64_t amount, PackedOutIndex output, uint32_t blockIndex);

        enum class OutputSearchResult : uint8_t
        {
            FOUND,
            NOT_FOUND,
            INVALID_ARGUMENT
        };

        TransactionValidatorState fillOutputsSpentByBlock(uint32_t blockIndex) const;

        uint8_t getBlockMajorVersionForHeight(uint32_t height) const;
        void fixChildrenParent(IBlockchainCache *p);

        void doPushBlock(const CachedBlock &cachedBlock,
                         const std::vector<CachedTransaction> &cachedTransactions,
                         const TransactionValidatorState &validatorState,
                         size_t blockSize,
                         uint64_t generatedCoins,
                         uint64_t blockDifficulty,
                         RawBlock &&rawBlock);
    };

}
