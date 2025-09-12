#ifndef BITCOIN_DIVIDEND_DIVIDEND_H
#define BITCOIN_DIVIDEND_DIVIDEND_H

#include <consensus/amount.h>
#include <serialize.h>
#include <map>
#include <string>

struct StakeInfo {
    CAmount weight{0};
    int last_payout_height{0};

    SERIALIZE_METHODS(StakeInfo, obj)
    {
        READWRITE(obj.weight, obj.last_payout_height);
    }
};

namespace dividend {
using Payouts = std::map<std::string, CAmount>;

//! Number of blocks in a quarter and year for dividend calculations.
inline constexpr int QUARTER_BLOCKS{16200};
inline constexpr int YEAR_BLOCKS{QUARTER_BLOCKS * 4};

/**
 * Determine the annual percentage rate for a given stake.
 * The rate increases from 1% up to 10% based on stake age and amount.
 */
double CalculateApr(CAmount amount, int blocks_held);

/**
 * Calculate dividend payouts for a set of stakes. Payouts occur only on
 * quarter boundaries and are capped by the available pool.
 * @param stakes map of address to StakeInfo
 * @param height current block height
 * @param pool total dividend pool to distribute
 * @return map of address to payout amount
 */
Payouts CalculatePayouts(const std::map<std::string, StakeInfo>& stakes, int height, CAmount pool);

} // namespace dividend

#endif // BITCOIN_DIVIDEND_DIVIDEND_H
