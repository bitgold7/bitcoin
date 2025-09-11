#include <dividend/dividend.h>
#include <algorithm>

namespace dividend {

Payouts CalculatePayouts(const std::map<std::string, StakeInfo>& stakes, int height, CAmount pool)
{
    Payouts payouts;
    if (pool <= 0) return payouts;
    long long total_weight_time = 0;
    for (const auto& [addr, info] : stakes) {
        if (info.weight < MIN_STAKE_WEIGHT) continue;
        if (height - info.start_height < MIN_STAKE_DURATION) continue;
        int effective_start = std::max(info.start_height, info.last_payout_height);
        int duration = height - effective_start;
        if (duration > 0) {
            total_weight_time += static_cast<long long>(info.weight) * duration;
        }
    }
    if (total_weight_time == 0) return payouts;
    for (const auto& [addr, info] : stakes) {
        if (info.weight < MIN_STAKE_WEIGHT) continue;
        if (height - info.start_height < MIN_STAKE_DURATION) continue;
        int effective_start = std::max(info.start_height, info.last_payout_height);
        int duration = height - effective_start;
        if (duration > 0) {
            CAmount share = pool * static_cast<long long>(info.weight) * duration / total_weight_time;
            if (share > 0) {
                payouts.emplace(addr, share);
            }
        }
    }
    return payouts;
}

} // namespace dividend
