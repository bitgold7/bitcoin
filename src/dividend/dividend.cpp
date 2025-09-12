#include <dividend/dividend.h>

#include <algorithm>
#include <vector>

namespace dividend {

double CalculateApr(CAmount amount, int blocks_held)
{
    double age_factor = std::min(1.0, static_cast<double>(blocks_held) / YEAR_BLOCKS);
    double amount_factor = std::min(1.0, static_cast<double>(amount) / (1000.0 * COIN));
    double factor = std::max(age_factor, amount_factor);
    return 0.01 + 0.09 * factor;
}

Payouts CalculatePayouts(const std::map<std::string, StakeInfo>& stakes, int height, CAmount pool)
{
    Payouts payouts;
    if (pool <= 0 || height % QUARTER_BLOCKS != 0) return payouts;
    CAmount total_desired = 0;
    struct Pending { std::string addr; CAmount desired; };
    std::vector<Pending> pending;
    for (const auto& [addr, info] : stakes) {
        int duration = height - info.last_payout_height;
        if (duration <= 0 || info.weight <= 0) continue;
        double apr = CalculateApr(info.weight, duration);
        CAmount desired = static_cast<CAmount>(info.weight * apr / 4.0);
        if (desired <= 0) continue;
        pending.push_back({addr, desired});
        total_desired += desired;
    }
    if (total_desired <= 0) return payouts;
    double scale = total_desired > pool ? static_cast<double>(pool) / total_desired : 1.0;
    for (const auto& p : pending) {
        CAmount payout = static_cast<CAmount>(p.desired * scale);
        if (payout > 0) {
            payouts.emplace(p.addr, payout);
        }
    }
    return payouts;
}

CMutableTransaction BuildPayoutTx(const std::map<std::string, StakeInfo>& stakes, int height, CAmount pool)
{
    CMutableTransaction tx;
    Payouts payouts = CalculatePayouts(stakes, height, pool);
    if (payouts.empty()) return tx;
    // Non-coinbase placeholder input to avoid IsCoinBase
    tx.vin.emplace_back(COutPoint(uint256(), 0));
    std::vector<std::pair<std::string, CAmount>> recipients(payouts.begin(), payouts.end());
    std::sort(recipients.begin(), recipients.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    CAmount paid{0};
    for (const auto& [addr, amt] : recipients) {
        (void)addr;
        tx.vout.emplace_back(amt, GetDividendScript());
        paid += amt;
    }
    if (pool > paid) {
        tx.vout.emplace_back(pool - paid, GetDividendScript());
    }
    return tx;
}

} // namespace dividend
