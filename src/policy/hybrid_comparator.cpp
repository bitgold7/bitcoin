#include <policy/hybrid_comparator.h>

#include <kernel/mempool_entry.h>
#include <node/miner.h>
#include <util/time.h>

FeeAgeStakeMetrics GetFeeAgeStakeMetrics(const CTxMemPoolEntry& entry)
{
    FeeAgeStakeMetrics m;
    m.fee = entry.GetFee();
    m.age = GetTime() - entry.GetTime().count();
    m.stake = 0;
    for (const auto& out : entry.GetTx().vout) {
        m.stake += out.nValue;
    }
    return m;
}

FeeAgeStakeMetrics GetFeeAgeStakeMetrics(const CTxMemPoolModifiedEntry& entry)
{
    return GetFeeAgeStakeMetrics(*entry.iter);
}
