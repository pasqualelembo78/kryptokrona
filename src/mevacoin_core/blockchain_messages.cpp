// Copyright (c) 2012-2017, The CryptoNote developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero Project
// Copyright (c) 2018-2019, The TurtleCoin Developers
// Copyright (c) 2019, The Kryptokrona Developers
//
// Please see the included LICENSE file for more information.

#include "mevacoin_core/blockchain_messages.h"

namespace mevacoin
{

    BlockchainMessage::BlockchainMessage(const NewBlock &message) : type(Type::NewBlock), newBlock(std::move(message))
    {
    }

    BlockchainMessage::BlockchainMessage(const NewAlternativeBlock &message)
        : type(Type::NewAlternativeBlock), newAlternativeBlock(message)
    {
    }

    BlockchainMessage::BlockchainMessage(const ChainSwitch &message)
        : type(Type::ChainSwitch), chainSwitch(new ChainSwitch(message))
    {
    }

    BlockchainMessage::BlockchainMessage(const AddTransaction &message)
        : type(Type::AddTransaction), addTransaction(new AddTransaction(message))
    {
    }

    BlockchainMessage::BlockchainMessage(const DeleteTransaction &message)
        : type(Type::DeleteTransaction), deleteTransaction(new DeleteTransaction(message))
    {
    }

    BlockchainMessage::BlockchainMessage(const BlockchainMessage &other) : type(other.type)
    {
        switch (type)
        {
        case Type::NewBlock:
            new (&newBlock) NewBlock(other.newBlock);
            break;
        case Type::NewAlternativeBlock:
            new (&newAlternativeBlock) NewAlternativeBlock(other.newAlternativeBlock);
            break;
        case Type::ChainSwitch:
            chainSwitch = new ChainSwitch(*other.chainSwitch);
            break;
        case Type::AddTransaction:
            addTransaction = new AddTransaction(*other.addTransaction);
            break;
        case Type::DeleteTransaction:
            deleteTransaction = new DeleteTransaction(*other.deleteTransaction);
            break;
        }
    }

    BlockchainMessage::~BlockchainMessage()
    {
        switch (type)
        {
        case Type::NewBlock:
            newBlock.~NewBlock();
            break;
        case Type::NewAlternativeBlock:
            newAlternativeBlock.~NewAlternativeBlock();
            break;
        case Type::ChainSwitch:
            delete chainSwitch;
            break;
        case Type::AddTransaction:
            delete addTransaction;
            break;
        case Type::DeleteTransaction:
            delete deleteTransaction;
            break;
        }
    }

    BlockchainMessage::Type BlockchainMessage::getType() const
    {
        return type;
    }

    auto BlockchainMessage::getNewBlock() const -> const NewBlock &
    {
        assert(getType() == Type::NewBlock);
        return newBlock;
    }

    auto BlockchainMessage::getNewAlternativeBlock() const -> const NewAlternativeBlock &
    {
        assert(getType() == Type::NewAlternativeBlock);
        return newAlternativeBlock;
    }

    auto BlockchainMessage::getChainSwitch() const -> const ChainSwitch &
    {
        assert(getType() == Type::ChainSwitch);
        return *chainSwitch;
    }

    BlockchainMessage makeChainSwitchMessage(uint32_t index, std::vector<crypto::Hash> &&hashes)
    {
        return BlockchainMessage{Messages::ChainSwitch{index, std::move(hashes)}};
    }

    BlockchainMessage makeNewAlternativeBlockMessage(uint32_t index, const crypto::Hash &hash)
    {
        return BlockchainMessage{Messages::NewAlternativeBlock{index, std::move(hash)}};
    }

    BlockchainMessage makeNewBlockMessage(uint32_t index, const crypto::Hash &hash)
    {
        return BlockchainMessage{Messages::NewBlock{index, std::move(hash)}};
    }

    BlockchainMessage makeAddTransactionMessage(std::vector<crypto::Hash> &&hashes)
    {
        return BlockchainMessage{Messages::AddTransaction{std::move(hashes)}};
    }

    BlockchainMessage makeDelTransactionMessage(std::vector<crypto::Hash> &&hashes,
                                                Messages::DeleteTransaction::Reason reason)
    {
        return BlockchainMessage{Messages::DeleteTransaction{std::move(hashes), reason}};
    }

    void BlockchainMessage::match(std::function<void(const NewBlock &)> newBlockVisitor,
                                  std::function<void(const NewAlternativeBlock &)> newAlternativeBlockVisitor,
                                  std::function<void(const ChainSwitch &)> chainSwitchMessageVisitor,
                                  std::function<void(const AddTransaction &)> addTxVisitor,
                                  std::function<void(const DeleteTransaction &)> delTxVisitor) const
    {
        switch (getType())
        {
        case Type::NewBlock:
            newBlockVisitor(newBlock);
            break;
        case Type::NewAlternativeBlock:
            newAlternativeBlockVisitor(newAlternativeBlock);
            break;
        case Type::ChainSwitch:
            chainSwitchMessageVisitor(*chainSwitch);
            break;
        case Type::AddTransaction:
            addTxVisitor(*addTransaction);
            break;
        case Type::DeleteTransaction:
            delTxVisitor(*deleteTransaction);
            break;
        }
    }
}
