#include <kernel/stake.h>

#include <arith_uint256.h>
#include <hash.h>
#include <pubkey.h>
#include <script/script.h>
#include <util/time.h>
#include <validation.h>

namespace kernel {

namespace {
uint256 g_last_modifier;
}

uint256 ComputeStakeModifierV3(const CBlockIndex* pindexPrev, const uint256& prev_modifier)
{
    HashWriter ss;
    ss << prev_modifier;
    if (pindexPrev) {
        ss << pindexPrev->GetBlockHash() << pindexPrev->nHeight << pindexPrev->nTime;
    }
    return ss.GetHash();
}

static uint256 ComputeKernelHash(const uint256& stake_modifier,
                                 const COutPoint& prevout,
                                 unsigned int nTimeBlockFrom,
                                 unsigned int nTimeTx)
{
    HashWriter ss;
    ss << stake_modifier << prevout.hash << prevout.n << nTimeBlockFrom << nTimeTx;
    return ss.GetHash();
}

bool CheckStakeKernelHash(const CBlockIndex* pindexPrev, unsigned int nBits,
                          uint256 hashBlockFrom, unsigned int nTimeBlockFrom,
                          CAmount amount, const COutPoint& prevout,
                          unsigned int nTimeTx, uint256& hashProofOfStake,
                          const Consensus::Params& params)
{
    if (!pindexPrev) return false;
    if (nTimeTx <= nTimeBlockFrom) return false;
    if ((nTimeTx & params.nStakeTimestampMask) != 0) return false;

    bool fNegative = false;
    bool fOverflow = false;
    arith_uint256 bnTarget;
    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);
    if (fNegative || fOverflow || bnTarget == 0) return false;

    arith_uint256 bnWeight(amount);
    bnTarget *= bnWeight;

    g_last_modifier = ComputeStakeModifierV3(pindexPrev, g_last_modifier);
    hashProofOfStake = ComputeKernelHash(g_last_modifier, prevout, nTimeBlockFrom, nTimeTx);
    return UintToArith256(hashProofOfStake) <= bnTarget;
}

bool IsProofOfStake(const CBlock& block)
{
    if (block.vtx.size() < 2) return false;
    const CTransaction& tx = *block.vtx[1];
    if (tx.IsCoinBase()) return false;
    if (tx.vin.empty() || tx.vout.size() < 2) return false;
    if (!tx.vout[0].scriptPubKey.empty()) return false;
    return true;
}

bool CheckBlockSignature(const CBlock& block)
{
    if (!IsProofOfStake(block)) {
        return block.vchBlockSig.empty();
    }
    if (block.vchBlockSig.empty()) return false;

    const CTransaction& tx{*block.vtx[1]};
    if (tx.vin.empty()) return false;
    const CScript& scriptSig{tx.vin[0].scriptSig};
    CScript::const_iterator it = scriptSig.begin();
    opcodetype op;
    std::vector<unsigned char> vchSig;
    std::vector<unsigned char> vchPub;
    if (!scriptSig.GetOp(it, op, vchSig)) return false;
    if (!scriptSig.GetOp(it, op, vchPub)) return false;
    CPubKey pubkey(vchPub);
    if (!pubkey.IsValid()) return false;
    return pubkey.Verify(block.GetHash(), block.vchBlockSig);
}

bool ContextualCheckProofOfStake(const CBlock& block,
                                 const CBlockIndex* pindexPrev,
                                 const CCoinsViewCache& view,
                                 const CChain& chain,
                                 const Consensus::Params& params)
{
    if (!pindexPrev) return false;
    if (!IsProofOfStake(block)) return false;

    const CTransaction& coinstake{*block.vtx[1]};
    if (block.nTime != coinstake.nLockTime) return false;
    if ((block.nTime & params.nStakeTimestampMask) != 0) return false;
    if (coinstake.vin.empty()) return false;

    const CTxIn& txin{coinstake.vin[0]};
    const Coin& coin = view.AccessCoin(txin.prevout);
    if (coin.IsSpent()) return false;
    const CBlockIndex* pindexFrom = chain[coin.nHeight];
    if (!pindexFrom) return false;
    unsigned int nTimeBlockFrom = pindexFrom->GetBlockTime();
    uint256 hashProofOfStake;
    if (!CheckStakeKernelHash(pindexPrev, block.nBits,
                              pindexFrom->GetBlockHash(), nTimeBlockFrom,
                              coin.out.nValue, txin.prevout,
                              block.nTime, hashProofOfStake, params)) {
        return false;
    }
    return true;
}

} // namespace kernel
