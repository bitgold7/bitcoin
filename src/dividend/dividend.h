#ifndef BITCOIN_DIVIDEND_DIVIDEND_H
#define BITCOIN_DIVIDEND_DIVIDEND_H

#include <consensus/amount.h>
#include <consensus/dividends/schedule.h>
#include <serialize.h>
#include <primitives/transaction.h>
#include <map>
#include <script/script.h>
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
inline constexpr int QUARTER_BLOCKS{consensus::dividends::SNAPSHOT_INTERVAL};
inline constexpr int YEAR_BLOCKS{QUARTER_BLOCKS * 4};
inline constexpr int BASIS_POINTS{10000};

/**
 * Determine the annual percentage rate for a given stake, expressed in basis
 * points (1% = 100 basis points). Uses only integer arithmetic and limits the
 * rate between 1% and 10% based on stake age and amount.
 */
int CalculateAprBasisPoints(CAmount amount, int blocks_held);

/**
 * Calculate dividend payouts for a set of stakes. Payouts occur only on
 * quarter boundaries and are capped by the available pool.
 * @param stakes map of address to StakeInfo
 * @param height current block height
 * @param pool total dividend pool to distribute
 * @return map of address to payout amount
 */
Payouts CalculatePayouts(const std::map<std::string, StakeInfo>& stakes, int height, CAmount pool);

/** Build a deterministic payout transaction.
 * Recipients are sorted by address and a final change output returns
 * any remaining balance to the dividend pool script.
 */
CMutableTransaction BuildPayoutTx(const std::map<std::string, StakeInfo>& stakes, int height, CAmount pool);

//! Constant script used for dividend pool outputs
inline const CScript& GetDividendScript()
{
    static const CScript script{CScript() << OP_TRUE};
    return script;
}

} // namespace dividend

#endif // BITCOIN_DIVIDEND_DIVIDEND_H
