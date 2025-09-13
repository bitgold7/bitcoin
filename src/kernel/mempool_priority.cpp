#include <kernel/mempool_priority.h>
#include <algorithm>

int64_t CalculateStakePriority(CAmount nStakeAmount)
{
    if (nStakeAmount >= 1000 * COIN) {
        return STAKE_PRIORITY_POINTS;
    }
    return (nStakeAmount / COIN) * STAKE_PRIORITY_SCALE;
}

int64_t CalculateFeePriority(CAmount nFee)
{
    if (nFee < 100) {
        return 0;
    }
    int64_t priority = 1 + (nFee - 100) / 1000;
    return (priority > FEE_PRIORITY_POINTS) ? FEE_PRIORITY_POINTS : priority;
}

int64_t CalculateStakeDurationPriority(int64_t nStakeDuration)
{
    if (nStakeDuration >= STAKE_DURATION_30_DAYS) {
        return STAKE_DURATION_30_DAYS_POINTS;
    }
    if (nStakeDuration >= STAKE_DURATION_7_DAYS) {
        return STAKE_DURATION_7_DAYS_POINTS;
    }
    return 0;
}

int64_t CalculateRbfCfpPriority(bool signals_rbf, bool has_ancestors)
{
    int64_t priority = 0;
    if (signals_rbf) {
        priority += RBF_PRIORITY_PENALTY;
    }
    if (has_ancestors) {
        priority += CPFP_PRIORITY_BONUS;
    }
    return priority;
}

int64_t ApplyPriorityDoSLimit(int64_t priority, int64_t max_priority)
{
    return std::clamp<int64_t>(priority, -max_priority, max_priority);
}

