// Copyright (c) 2012-2017, The MevaCoin developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero Project
// Copyright (c) 2018-2019, The TurtleCoin Developers
// Copyright (c) 2019, The Kryptokrona Developers
//
// Please see the included LICENSE file for more information.

#include <string.h>
#include "context.h"

void makecontext(uctx *ucp, void (*func)(void), intptr_t arg)
{
    long *sp;

    memset(&ucp->uc_mcontext, 0, sizeof ucp->uc_mcontext);
    ucp->uc_mcontext.mc_rdi = (long)arg;
    sp = (long *)ucp->uc_stack.ss_sp + ucp->uc_stack.ss_size / sizeof(long);
    sp -= 1;
    sp = (void *)((uintptr_t)sp - (uintptr_t)sp % 16); /* 16-align for OS X */
    *--sp = 0;                                         /* return address */
    ucp->uc_mcontext.mc_rip = (long)func;
    ucp->uc_mcontext.mc_rsp = (long)sp;
}

int swapcontext(uctx *oucp, const uctx *ucp)
{
    if (getcontext(oucp) == 0)
        setcontext(ucp);
    return 0;
}
