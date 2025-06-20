// Copyright (c) 2012-2017, The MevaCoin developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero Project
// Copyright (c) 2018-2019, The TurtleCoin Developers
// Copyright (c) 2019, The Kryptokrona Developers
//
// Please see the included LICENSE file for more information.

#pragma once
#include <chrono>
#include <syst/context_group.h>
#include <syst/timer.h>

namespace syst
{

    class ContextGroupTimeout
    {
    public:
        ContextGroupTimeout(Dispatcher &, ContextGroup &, std::chrono::nanoseconds);

    private:
        Timer timeoutTimer;
        ContextGroup workingContextGroup;
    };

}
