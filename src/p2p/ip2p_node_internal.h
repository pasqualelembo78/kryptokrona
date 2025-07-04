// Copyright (c) 2012-2017, The CryptoNote developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero Project
// Copyright (c) 2018-2019, The TurtleCoin Developers
// Copyright (c) 2019, The Kryptokrona Developers
//
// Please see the included LICENSE file for more information.

#pragma once

#include <list>
#include "p2p_protocol_definitions.h"

namespace mevacoin
{

    class P2pContext;

    class IP2pNodeInternal
    {
    public:
        virtual const CORE_SYNC_DATA &getGenesisPayload() const = 0;
        virtual std::list<PeerlistEntry> getLocalPeerList() = 0;
        virtual basic_node_data getNodeData() const = 0;
        virtual uint64_t getPeerId() const = 0;

        virtual void handleNodeData(const basic_node_data &node, P2pContext &ctx) = 0;
        virtual bool handleRemotePeerList(const std::list<PeerlistEntry> &peerlist, time_t local_time) = 0;
        virtual void tryPing(P2pContext &ctx) = 0;
    };

}
