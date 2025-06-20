// Copyright (c) 2012-2017, The MevaCoin developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero Project
// Copyright (c) 2018-2019, The TurtleCoin Developers
// Copyright (c) 2019, The Kryptokrona Developers
//
// Please see the included LICENSE file for more information.

#include "signal_handler.h"

#include <mutex>
#include <iostream>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#else
#include <signal.h>
#include <cstring>
#endif

namespace
{

    std::function<void(void)> m_handler;

    void handleSignal()
    {
        static std::mutex m_mutex;
        std::unique_lock<std::mutex> lock(m_mutex, std::try_to_lock);
        if (!lock.owns_lock())
        {
            return;
        }

        if (m_handler)
        {
            m_handler();
        }
    }

#if defined(WIN32)
    BOOL WINAPI winHandler(DWORD type)
    {
        if (CTRL_C_EVENT == type || CTRL_BREAK_EVENT == type)
        {
            handleSignal();
            return TRUE;
        }
        else
        {
            std::cerr << "Got control signal " << type << ". Exiting without saving...";
            return FALSE;
        }
        return TRUE;
    }

#else

    void posixHandler(int /*type*/)
    {
        handleSignal();
    }
#endif

}

namespace tools
{

    bool SignalHandler::install(std::function<void(void)> t)
    {
#if defined(WIN32)
        bool r = TRUE == ::SetConsoleCtrlHandler(&winHandler, TRUE);
        if (r)
        {
            m_handler = t;
        }
        return r;
#else
        struct sigaction newMask;
        std::memset(&newMask, 0, sizeof(struct sigaction));
        newMask.sa_handler = posixHandler;
        if (sigaction(SIGINT, &newMask, nullptr) != 0)
        {
            return false;
        }

        if (sigaction(SIGTERM, &newMask, nullptr) != 0)
        {
            return false;
        }

        std::memset(&newMask, 0, sizeof(struct sigaction));
        newMask.sa_handler = SIG_IGN;
        if (sigaction(SIGPIPE, &newMask, nullptr) != 0)
        {
            return false;
        }

        m_handler = t;
        return true;
#endif
    }
}
