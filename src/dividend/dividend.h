#ifndef BITCOIN_DIVIDEND_DIVIDEND_H
#define BITCOIN_DIVIDEND_DIVIDEND_H

#include <consensus/amount.h>
#include <map>
#include <string>

struct StakeInfo {
    CAmount weight{0};
    int last_payout_height{0};
};

namespace dividend {
using Payouts = std::map<std::string, CAmount>;

/**
 * Calculate dividend payouts for a set of stakes.
 * @param stakes map of address to StakeInfo
 * @param height current block height
 * @param pool total dividend pool to distribute
 * @return map of address to payout amount
 */
Payouts CalculatePayouts(const std::map<std::string, StakeInfo>& stakes, int height, CAmount pool);

} // namespace dividend

#endif // BITCOIN_DIVIDEND_DIVIDEND_H
