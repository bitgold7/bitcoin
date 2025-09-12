#ifndef BITCOIN_KERNEL_STAKE_H
#define BITCOIN_KERNEL_STAKE_H

#include <consensus/amount.h>
#include <consensus/params.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <uint256.h>
#include <coins.h>
#include <chain.h>
#include <util/time.h>

namespace kernel {

/** Compute stake modifier version 3.
 *  The modifier mixes the previous modifier with key fields from the
 *  previous block to make staking deterministic while preventing grinding.
 */
uint256 ComputeStakeModifierV3(const CBlockIndex* pindexPrev, const uint256& prev_modifier);

/** Check that a stake kernel hash meets the target specified by nBits. */
bool CheckStakeKernelHash(const CBlockIndex* pindexPrev, unsigned int nBits,
                          uint256 hashBlockFrom, unsigned int nTimeBlockFrom,
                          CAmount amount, const COutPoint& prevout,
                          unsigned int nTimeTx, uint256& hashProofOfStake,
                          const Consensus::Params& params);

/** Returns true if the block contains a valid coinstake transaction. */
bool IsProofOfStake(const CBlock& block);

/** Verify the signature on a proof-of-stake block. */
bool CheckBlockSignature(const CBlock& block);

/** Contextual checks for a proof-of-stake block. */
bool ContextualCheckProofOfStake(const CBlock& block,
                                 const CBlockIndex* pindexPrev,
                                 const CCoinsViewCache& view,
                                 const CChain& chain,
                                 const Consensus::Params& params);

/** Basic timestamp checks for a staked block. */
inline bool CheckStakeTimestamp(const CBlockHeader& h, const Consensus::Params& p)
{
    if ((h.nTime & p.nStakeTimestampMask) != 0) return false;
    if (h.nTime > GetTime() + 15) return false;
    return true;
}

} // namespace kernel

#endif // BITCOIN_KERNEL_STAKE_H
