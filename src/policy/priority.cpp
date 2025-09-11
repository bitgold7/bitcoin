#include <policy/priority.h>

#include <coins.h>
#include <kernel/mempool_priority.h>
#include <policy/policy.h>
#include <txmempool.h>

#include <algorithm>

int64_t GetBGDPriority(const CTransaction& tx, CAmount fee,
                       const CCoinsViewCache& view, const CTxMemPool& pool,
                       int chain_height, const Consensus::Params& params,
                       bool signals_rbf, bool has_ancestors)
{
    int64_t priority{0};
    priority += CalculateStakePriority(tx.GetValueOut());
    priority += CalculateFeePriority(fee);
    int64_t stake_duration{0};
    for (const CTxIn& txin : tx.vin) {
        const Coin& coin = view.AccessCoin(txin.prevout);
        if (!coin.IsSpent()) {
            int n_blocks = chain_height - static_cast<int>(coin.nHeight);
            if (n_blocks > 0) {
                stake_duration = std::max<int64_t>(stake_duration,
                                                   int64_t(n_blocks) * params.nPowTargetSpacing);
            }
        }
    }
    priority += CalculateStakeDurationPriority(stake_duration);
    if (pool.DynamicMemoryUsage() > pool.m_opts.max_size_bytes * 9 / 10) {
        priority += CONGESTION_PENALTY;
    }
    priority += CalculateRbfCfpPriority(signals_rbf, has_ancestors);
    priority = ApplyPriorityDoSLimit(priority, g_max_priority);
    return priority;
}
