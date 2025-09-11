#include <dividend/dividend.h>

namespace dividend {

Payouts CalculatePayouts(const std::map<std::string, StakeInfo>& stakes, int height, CAmount pool)
{
    Payouts payouts;
    if (pool <= 0) return payouts;
    long long total_weight_time = 0;
    for (const auto& [addr, info] : stakes) {
        int duration = height - info.last_payout_height;
        if (duration > 0 && info.weight > 0) {
            total_weight_time += static_cast<long long>(info.weight) * duration;
        }
    }
    if (total_weight_time == 0) return payouts;
    for (const auto& [addr, info] : stakes) {
        int duration = height - info.last_payout_height;
        if (duration > 0 && info.weight > 0) {
            CAmount share = pool * static_cast<long long>(info.weight) * duration / total_weight_time;
            if (share > 0) {
                payouts.emplace(addr, share);
            }
        }
    }
    return payouts;
}

} // namespace dividend
