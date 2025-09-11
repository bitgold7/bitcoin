#ifndef BITCOIN_POS_STAKE_H
#define BITCOIN_POS_STAKE_H

#include <consensus/amount.h>
#include <chain.h>
#include <coins.h>
#include <consensus/params.h>
#include <primitives/block.h>
#include <uint256.h>
#include <util/time.h>

class CBlockIndex;

// Default timestamp granularity for staked blocks (16 seconds, PoSV3.1)
static constexpr unsigned int STAKE_TIMESTAMP_MASK = 0xF;
// Default minimum coin age for staking (1 hour, PoSV3.1)
// Network-specific values are provided via consensus parameters
static constexpr int64_t MIN_STAKE_AGE = 60 * 60;

/** Check that the kernel for a stake meets the required target */
bool CheckStakeKernelHash(const CBlockIndex* pindexPrev, unsigned int nBits,
                          uint256 hashBlockFrom, unsigned int nTimeBlockFrom,
                          CAmount amount, const COutPoint& prevout,
                          unsigned int nTimeTx, uint256& hashProofOfStake,
                          bool fPrintProofOfStake,
                          const Consensus::Params& params);

/**
 * Validate the proof-of-stake for a block using contextual chain information.
 * The block must include a coinstake transaction whose input exists in the
 * UTXO set and has matured enough time. The stake kernel hash must satisfy the
 * network difficulty.
 */
bool ContextualCheckProofOfStake(const CBlock& block, const CBlockIndex* pindexPrev,
                                 const CCoinsViewCache& view, const CChain& chain,
                                 const Consensus::Params& params);

/**
 * Compute the proof-of-stake reward for a block at a given height. The reward
 * consists of the per-block subsidy (inflation) plus the validator's portion
 * of transaction fees (currently 90%).
 * @param[out] dividend_portion amount to allocate to dividend pool
 */
CAmount GetProofOfStakeReward(int nHeight, CAmount nFees, int64_t coin_age_weight, const Consensus::Params& params, CAmount& dividend_portion);

/** Return true if the block appears to be proof-of-stake. */
bool IsProofOfStake(const CBlock& block);

/** Verify the signature on a proof-of-stake block. */
bool CheckBlockSignature(const CBlock& block);

inline bool CheckStakeTimestamp(const CBlockHeader& h, const Consensus::Params& p)
{
    if ((h.nTime & p.nStakeTimestampMask) != 0) return false;
    if (h.nTime > GetTime() + 15) return false;
    return true;
}

#endif // BITCOIN_POS_STAKE_H
