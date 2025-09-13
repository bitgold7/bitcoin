// Bitcoin Core's dividend module uses integer arithmetic to avoid floating
// point rounding errors when calculating payouts.

#include <consensus/dividends/dividend.h>

#include <algorithm>
#include <vector>

namespace dividend {

int CalculateAprBasisPoints(CAmount amount, int blocks_held)
{
    // Age and amount factors scaled to BASIS_POINTS precision
    int64_t age_factor = std::min<int64_t>(blocks_held, YEAR_BLOCKS) * BASIS_POINTS / YEAR_BLOCKS;
    int64_t amount_factor = std::min<CAmount>(amount, 1000 * COIN) * BASIS_POINTS / (1000 * COIN);
    int64_t factor = std::max(age_factor, amount_factor);
    // Base rate is 1% (100 basis points) plus up to 9% (900 bps)
    return 100 + (900 * factor) / BASIS_POINTS;
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
        int apr_bp = CalculateAprBasisPoints(info.weight, duration);
        CAmount desired = info.weight * apr_bp / (4 * BASIS_POINTS);
        if (desired <= 0) continue;
        pending.push_back({addr, desired});
        total_desired += desired;
    }
    if (total_desired <= 0) return payouts;
    for (const auto& p : pending) {
        CAmount payout;
        if (total_desired <= pool) {
            payout = p.desired;
        } else {
            payout = pool * p.desired / total_desired;
        }
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
        // Return any leftover back to the dividend pool script
        tx.vout.emplace_back(pool - paid, GetDividendScript());
    }
    return tx;
}

} // namespace dividend
