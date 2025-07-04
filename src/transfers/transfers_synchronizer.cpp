// Copyright (c) 2012-2017, The CryptoNote developers, The Bytecoin developers
// Copyright (c) 2018, The BBSCoin Developers
// Copyright (c) 2018, The Karbo Developers
// Copyright (c) 2018-2019, The TurtleCoin Developers
// Copyright (c) 2019, The Kryptokrona Developers
//
// Please see the included LICENSE file for more information.

#include "transfers_synchronizer.h"
#include "transfers_consumer.h"

#include "common/std_input_stream.h"
#include "common/std_output_stream.h"
#include "mevacoin_core/mevacoin_basic_impl.h"
#include "serialization/binary_input_stream_serializer.h"
#include "serialization/binary_output_stream_serializer.h"

using namespace common;
using namespace crypto;

namespace mevacoin
{

    const uint32_t TRANSFERS_STORAGE_ARCHIVE_VERSION = 0;

    TransfersSyncronizer::TransfersSyncronizer(const mevacoin::Currency &currency, std::shared_ptr<logging::ILogger> logger, IBlockchainSynchronizer &sync, INode &node) : m_currency(currency), m_logger(logger, "TransfersSyncronizer"), m_sync(sync), m_node(node)
    {
    }

    TransfersSyncronizer::~TransfersSyncronizer()
    {
        m_sync.stop();
        for (const auto &kv : m_consumers)
        {
            m_sync.removeConsumer(kv.second.get());
        }
    }

    void TransfersSyncronizer::initTransactionPool(const std::unordered_set<crypto::Hash> &uncommitedTransactions)
    {
        for (auto it = m_consumers.begin(); it != m_consumers.end(); ++it)
        {
            it->second->initTransactionPool(uncommitedTransactions);
        }
    }

    ITransfersSubscription &TransfersSyncronizer::addSubscription(const AccountSubscription &acc)
    {
        auto it = m_consumers.find(acc.keys.address.viewPublicKey);

        if (it == m_consumers.end())
        {
            std::unique_ptr<TransfersConsumer> consumer(
                new TransfersConsumer(m_currency, m_node, m_logger.getLogger(), acc.keys.viewSecretKey));

            m_sync.addConsumer(consumer.get());
            consumer->addObserver(this);
            it = m_consumers.insert(std::make_pair(acc.keys.address.viewPublicKey, std::move(consumer))).first;
        }

        return it->second->addSubscription(acc);
    }

    bool TransfersSyncronizer::removeSubscription(const AccountPublicAddress &acc)
    {
        auto it = m_consumers.find(acc.viewPublicKey);
        if (it == m_consumers.end())
            return false;

        if (it->second->removeSubscription(acc))
        {
            m_sync.removeConsumer(it->second.get());
            m_consumers.erase(it);

            m_subscribers.erase(acc.viewPublicKey);
        }

        return true;
    }

    void TransfersSyncronizer::getSubscriptions(std::vector<AccountPublicAddress> &subscriptions)
    {
        for (const auto &kv : m_consumers)
        {
            kv.second->getSubscriptions(subscriptions);
        }
    }

    ITransfersSubscription *TransfersSyncronizer::getSubscription(const AccountPublicAddress &acc)
    {
        auto it = m_consumers.find(acc.viewPublicKey);
        return (it == m_consumers.end()) ? nullptr : it->second->getSubscription(acc);
    }

    void TransfersSyncronizer::addPublicKeysSeen(const AccountPublicAddress &acc, const crypto::Hash &transactionHash, const crypto::PublicKey &outputKey)
    {
        auto it = m_consumers.find(acc.viewPublicKey);
        if (it != m_consumers.end())
        {
            it->second->addPublicKeysSeen(transactionHash, outputKey);
        }
    }

    std::vector<crypto::Hash> TransfersSyncronizer::getViewKeyKnownBlocks(const crypto::PublicKey &publicViewKey)
    {
        auto it = m_consumers.find(publicViewKey);
        if (it == m_consumers.end())
        {
            throw std::invalid_argument("Consumer not found");
        }

        return m_sync.getConsumerKnownBlocks(*it->second);
    }

    void TransfersSyncronizer::onBlocksAdded(IBlockchainConsumer *consumer, const std::vector<crypto::Hash> &blockHashes)
    {
        auto it = findSubscriberForConsumer(consumer);
        if (it != m_subscribers.end())
        {
            it->second->notify(&ITransfersSynchronizerObserver::onBlocksAdded, it->first, blockHashes);
        }
    }

    void TransfersSyncronizer::onBlockchainDetach(IBlockchainConsumer *consumer, uint32_t blockIndex)
    {
        auto it = findSubscriberForConsumer(consumer);
        if (it != m_subscribers.end())
        {
            it->second->notify(&ITransfersSynchronizerObserver::onBlockchainDetach, it->first, blockIndex);
        }
    }

    void TransfersSyncronizer::onTransactionDeleteBegin(IBlockchainConsumer *consumer, crypto::Hash transactionHash)
    {
        auto it = findSubscriberForConsumer(consumer);
        if (it != m_subscribers.end())
        {
            it->second->notify(&ITransfersSynchronizerObserver::onTransactionDeleteBegin, it->first, transactionHash);
        }
    }

    void TransfersSyncronizer::onTransactionDeleteEnd(IBlockchainConsumer *consumer, crypto::Hash transactionHash)
    {
        auto it = findSubscriberForConsumer(consumer);
        if (it != m_subscribers.end())
        {
            it->second->notify(&ITransfersSynchronizerObserver::onTransactionDeleteEnd, it->first, transactionHash);
        }
    }

    void TransfersSyncronizer::onTransactionUpdated(IBlockchainConsumer *consumer, const crypto::Hash &transactionHash,
                                                    const std::vector<ITransfersContainer *> &containers)
    {

        auto it = findSubscriberForConsumer(consumer);
        if (it != m_subscribers.end())
        {
            it->second->notify(&ITransfersSynchronizerObserver::onTransactionUpdated, it->first, transactionHash, containers);
        }
    }

    void TransfersSyncronizer::subscribeConsumerNotifications(const crypto::PublicKey &viewPublicKey, ITransfersSynchronizerObserver *observer)
    {
        auto it = m_subscribers.find(viewPublicKey);
        if (it != m_subscribers.end())
        {
            it->second->add(observer);
            return;
        }

        auto insertedIt = m_subscribers.emplace(viewPublicKey, std::unique_ptr<SubscribersNotifier>(new SubscribersNotifier())).first;
        insertedIt->second->add(observer);
    }

    void TransfersSyncronizer::unsubscribeConsumerNotifications(const crypto::PublicKey &viewPublicKey, ITransfersSynchronizerObserver *observer)
    {
        m_subscribers.at(viewPublicKey)->remove(observer);
    }

    void TransfersSyncronizer::save(std::ostream &os)
    {
        m_sync.save(os);

        StdOutputStream stream(os);
        mevacoin::BinaryOutputStreamSerializer s(stream);
        s(const_cast<uint32_t &>(TRANSFERS_STORAGE_ARCHIVE_VERSION), "version");

        uint64_t subscriptionCount = m_consumers.size();

        s.beginArray(subscriptionCount, "consumers");

        for (const auto &consumer : m_consumers)
        {
            s.beginObject("");
            s(const_cast<PublicKey &>(consumer.first), "view_key");

            std::stringstream consumerState;
            // synchronization state
            m_sync.getConsumerState(consumer.second.get())->save(consumerState);

            std::string blob = consumerState.str();
            s(blob, "state");

            std::vector<AccountPublicAddress> subscriptions;
            consumer.second->getSubscriptions(subscriptions);
            uint64_t subCount = subscriptions.size();

            s.beginArray(subCount, "subscriptions");

            for (auto &addr : subscriptions)
            {
                auto sub = consumer.second->getSubscription(addr);
                if (sub != nullptr)
                {
                    s.beginObject("");

                    std::stringstream subState;
                    assert(sub);
                    sub->getContainer().save(subState);
                    // store data block
                    std::string blob = subState.str();
                    s(addr, "address");
                    s(blob, "state");

                    s.endObject();
                }
            }

            s.endArray();
            s.endObject();
        }
    }

    namespace
    {
        std::string getObjectState(IStreamSerializable &obj)
        {
            std::stringstream stream;
            obj.save(stream);
            return stream.str();
        }

        void setObjectState(IStreamSerializable &obj, const std::string &state)
        {
            std::stringstream stream(state);
            obj.load(stream);
        }

    }

    void TransfersSyncronizer::load(std::istream &is)
    {
        m_sync.load(is);

        StdInputStream inputStream(is);
        mevacoin::BinaryInputStreamSerializer s(inputStream);
        uint32_t version = 0;

        s(version, "version");

        if (version > TRANSFERS_STORAGE_ARCHIVE_VERSION)
        {
            throw std::runtime_error("TransfersSyncronizer version mismatch");
        }

        struct ConsumerState
        {
            PublicKey viewKey;
            std::string state;
            std::vector<std::pair<AccountPublicAddress, std::string>> subscriptionStates;
        };

        std::vector<ConsumerState> updatedStates;

        try
        {
            uint64_t subscriptionCount = 0;
            s.beginArray(subscriptionCount, "consumers");

            while (subscriptionCount--)
            {
                s.beginObject("");
                PublicKey viewKey;
                s(viewKey, "view_key");

                std::string blob;
                s(blob, "state");

                auto subIter = m_consumers.find(viewKey);
                if (subIter != m_consumers.end())
                {
                    auto consumerState = m_sync.getConsumerState(subIter->second.get());
                    assert(consumerState);

                    {
                        // store previous state
                        auto prevConsumerState = getObjectState(*consumerState);
                        // load consumer state
                        setObjectState(*consumerState, blob);
                        updatedStates.push_back(ConsumerState{viewKey, std::move(prevConsumerState)});
                    }

                    // load subscriptions
                    uint64_t subCount = 0;
                    s.beginArray(subCount, "subscriptions");

                    while (subCount--)
                    {
                        s.beginObject("");

                        AccountPublicAddress acc;
                        std::string state;

                        s(acc, "address");
                        s(state, "state");

                        auto sub = subIter->second->getSubscription(acc);

                        if (sub != nullptr)
                        {
                            auto prevState = getObjectState(sub->getContainer());
                            setObjectState(sub->getContainer(), state);
                            updatedStates.back().subscriptionStates.push_back(std::make_pair(acc, prevState));
                        }
                        else
                        {
                            m_logger(logging::DEBUGGING) << "Subscription not found: " << m_currency.accountAddressAsString(acc);
                        }

                        s.endObject();
                    }

                    s.endArray();
                }
                else
                {
                    m_logger(logging::DEBUGGING) << "Consumer not found: " << viewKey;
                }

                s.endObject();
            }

            s.endArray();
        }
        catch (...)
        {
            // rollback state
            for (const auto &consumerState : updatedStates)
            {
                auto consumer = m_consumers.find(consumerState.viewKey)->second.get();
                setObjectState(*m_sync.getConsumerState(consumer), consumerState.state);
                for (const auto &sub : consumerState.subscriptionStates)
                {
                    setObjectState(consumer->getSubscription(sub.first)->getContainer(), sub.second);
                }
            }
            throw;
        }
    }

    bool TransfersSyncronizer::findViewKeyForConsumer(IBlockchainConsumer *consumer, crypto::PublicKey &viewKey) const
    {
        // since we have only couple of consumers linear complexity is fine
        auto it = std::find_if(m_consumers.begin(), m_consumers.end(), [consumer](const ConsumersContainer::value_type &subscription)
                               { return subscription.second.get() == consumer; });

        if (it == m_consumers.end())
        {
            return false;
        }

        viewKey = it->first;
        return true;
    }

    TransfersSyncronizer::SubscribersContainer::const_iterator TransfersSyncronizer::findSubscriberForConsumer(IBlockchainConsumer *consumer) const
    {
        crypto::PublicKey viewKey;
        if (findViewKeyForConsumer(consumer, viewKey))
        {
            auto it = m_subscribers.find(viewKey);
            if (it != m_subscribers.end())
            {
                return it;
            }
        }

        return m_subscribers.end();
    }

}
