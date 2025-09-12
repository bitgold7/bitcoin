#ifndef BITCOIN_POLICY_HYBRID_COMPARATOR_H
#define BITCOIN_POLICY_HYBRID_COMPARATOR_H

#include <consensus/amount.h>
#include <cstdint>

class CTxMemPoolEntry;
struct CTxMemPoolModifiedEntry;

struct FeeAgeStakeMetrics {
    CAmount fee;
    int64_t age;
    uint64_t stake;
};

FeeAgeStakeMetrics GetFeeAgeStakeMetrics(const CTxMemPoolEntry& entry);
FeeAgeStakeMetrics GetFeeAgeStakeMetrics(const CTxMemPoolModifiedEntry& entry);

class HybridComparator
{
public:
    template <typename T>
    bool operator()(const T& a, const T& b) const
    {
        const FeeAgeStakeMetrics m1 = GetFeeAgeStakeMetrics(a);
        const FeeAgeStakeMetrics m2 = GetFeeAgeStakeMetrics(b);
        if (m1.fee != m2.fee) return m1.fee > m2.fee;
        if (m1.age != m2.age) return m1.age > m2.age;
        return m1.stake > m2.stake;
    }
};

#endif // BITCOIN_POLICY_HYBRID_COMPARATOR_H
