// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <policy/hybrid_comparator.h>
#include <logging.h>

static HybridLogHook g_hybrid_log_hook;

void SetHybridComparatorLogHook(HybridLogHook hook)
{
    g_hybrid_log_hook = std::move(hook);
}

void LogHybridComparator(const std::string& message)
{
    LogPrintf("hybrid comparator: %s\n", message);
    if (g_hybrid_log_hook) g_hybrid_log_hook(message);
}
