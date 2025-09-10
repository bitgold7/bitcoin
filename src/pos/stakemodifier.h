#ifndef BITCOIN_POS_STAKEMODIFIER_H
#define BITCOIN_POS_STAKEMODIFIER_H

#include <uint256.h>

class CBlockIndex;

/**
 * Compute a new stake modifier using the previous modifier and block data.
 */
uint256 ComputeStakeModifier(const CBlockIndex* pindexPrev, const uint256& prev_modifier);

#endif // BITCOIN_POS_STAKEMODIFIER_H
