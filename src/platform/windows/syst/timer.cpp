// Copyright (c) 2012-2017, The MevaCoin developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero Project
// Copyright (c) 2018-2019, The TurtleCoin Developers
// Copyright (c) 2019, The Kryptokrona Developers
//
// Please see the included LICENSE file for more information.

#include "timer.h"
#include <cassert>
#include <string>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <syst/interrupted_exception.h>
#include "dispatcher.h"

namespace syst
{

    namespace
    {

        struct TimerContext
        {
            uint64_t time;
            NativeContext *context;
            bool interrupted;
        };

    }

    Timer::Timer() : dispatcher(nullptr)
    {
    }

    Timer::Timer(Dispatcher &dispatcher) : dispatcher(&dispatcher), context(nullptr)
    {
    }

    Timer::Timer(Timer &&other) : dispatcher(other.dispatcher)
    {
        if (dispatcher != nullptr)
        {
            assert(other.context == nullptr);
            context = nullptr;
            other.dispatcher = nullptr;
        }
    }

    Timer::~Timer()
    {
        assert(dispatcher == nullptr || context == nullptr);
    }

    Timer &Timer::operator=(Timer &&other)
    {
        assert(dispatcher == nullptr || context == nullptr);
        dispatcher = other.dispatcher;
        if (dispatcher != nullptr)
        {
            assert(other.context == nullptr);
            context = nullptr;
            other.dispatcher = nullptr;
        }

        return *this;
    }

    void Timer::sleep(std::chrono::nanoseconds duration)
    {
        assert(dispatcher != nullptr);
        assert(context == nullptr);
        if (dispatcher->interrupted())
        {
            throw InterruptedException();
        }

        LARGE_INTEGER frequency;
        LARGE_INTEGER ticks;
        QueryPerformanceCounter(&ticks);
        QueryPerformanceFrequency(&frequency);
        uint64_t currentTime = ticks.QuadPart / (frequency.QuadPart / 1000);
        uint64_t time = currentTime + duration.count() / 1000000;
        TimerContext timerContext{time, dispatcher->getCurrentContext(), false};
        context = &timerContext;
        dispatcher->addTimer(time, dispatcher->getCurrentContext());
        dispatcher->getCurrentContext()->interruptProcedure = [&]()
        {
            assert(dispatcher != nullptr);
            assert(context != nullptr);
            TimerContext *timerContext = static_cast<TimerContext *>(context);
            if (!timerContext->interrupted)
            {
                dispatcher->interruptTimer(timerContext->time, timerContext->context);
                timerContext->interrupted = true;
            }
        };

        dispatcher->dispatch();
        dispatcher->getCurrentContext()->interruptProcedure = nullptr;
        assert(timerContext.context == dispatcher->getCurrentContext());
        assert(dispatcher != nullptr);
        assert(context == &timerContext);
        context = nullptr;
        if (timerContext.interrupted)
        {
            throw InterruptedException();
        }
    }

}
