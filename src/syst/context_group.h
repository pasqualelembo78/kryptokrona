// Copyright (c) 2012-2017, The MevaCoin developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero Project
// Copyright (c) 2018-2019, The TurtleCoin Developers
// Copyright (c) 2019, The Kryptokrona Developers
//
// Please see the included LICENSE file for more information.

#pragma once

#include <syst/dispatcher.h>

namespace syst
{

    class ContextGroup
    {
    public:
        explicit ContextGroup(Dispatcher &dispatcher);
        ContextGroup(const ContextGroup &) = delete;
        ContextGroup(ContextGroup &&other);
        ~ContextGroup();
        ContextGroup &operator=(const ContextGroup &) = delete;
        ContextGroup &operator=(ContextGroup &&other);
        void interrupt();
        void spawn(std::function<void()> &&procedure);
        void wait();

    private:
        Dispatcher *dispatcher;
        NativeContextGroup contextGroup;
    };

}
