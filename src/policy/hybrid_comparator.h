// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#ifndef BITCOIN_POLICY_HYBRID_COMPARATOR_H
#define BITCOIN_POLICY_HYBRID_COMPARATOR_H

#include <functional>
#include <string>

/** Type for external log hooks used by the hybrid comparator. */
using HybridLogHook = std::function<void(const std::string&)>;

/** Set an external hook that will be invoked on comparator events. */
void SetHybridComparatorLogHook(HybridLogHook hook);

/** Emit a log message for hybrid comparator events. */
void LogHybridComparator(const std::string& message);

#endif // BITCOIN_POLICY_HYBRID_COMPARATOR_H
