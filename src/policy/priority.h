#ifndef BITCOIN_POLICY_PRIORITY_H
#define BITCOIN_POLICY_PRIORITY_H

#include <consensus/amount.h>
#include <consensus/params.h>
#include <cstdint>

class CTransaction;
class CCoinsViewCache;
class CTxMemPool;

/**
 * Compute the combined priority score for a transaction using stake
 * amount, fees, stake duration, congestion penalty and RBF/CPFP
 * adjustments.
 */
int64_t GetBGDPriority(const CTransaction& tx, CAmount fee,
                       const CCoinsViewCache& view, const CTxMemPool& pool,
                       int chain_height, const Consensus::Params& params,
                       bool signals_rbf, bool has_ancestors);

#endif // BITCOIN_POLICY_PRIORITY_H
