#include <node/stake_modifier_manager.h>
#include <pos/difficulty.h>
#include <pos/stake.h>
#include <pos/stakemodifier.h>

#include <algorithm>
#include <arith_uint256.h>
#include <hash.h>
#include <logging.h>
#include <primitives/transaction.h>
#include <pubkey.h>
#include <util/overflow.h>
#include <validation.h>

#include <cassert>
#include <set>

/**
 * Proof-of-Stake v3.1 implementation matching Blackcoin's security model.
 * Implements the stake modifier, reward clipping and Fakestake mitigations
 * to provide deterministic staking with bounded rewards and protection
 * against fake coinstake transactions.
 */

static bool IsCoinStakeTx(const CTransaction& tx)
{
    // A coinstake must: not be coinbase; have at least one input; have at least two outputs
    // and the first output must be empty (scriptPubKey.size()==0) per typical PoS v3 pattern.
    if (tx.IsCoinBase()) return false;
    if (tx.vin.empty()) return false;
    if (tx.vout.size() < 2) return false;
    if (!tx.vout[0].scriptPubKey.empty()) return false;
    return true;
}

bool IsProofOfStake(const CBlock& block)
{
    if (block.vtx.size() < 2) return false;
    return IsCoinStakeTx(*block.vtx[1]);
}

bool CheckBlockSignature(const CBlock& block)
{
    if (!IsProofOfStake(block)) {
        return block.vchBlockSig.empty();
    }

    if (block.vchBlockSig.empty()) {
        return false;
    }

    const CTransaction& tx{*block.vtx[1]};
    if (tx.vin.empty()) {
        return false;
    }

    const CScript& scriptSig{tx.vin[0].scriptSig};
    CScript::const_iterator it = scriptSig.begin();
    std::vector<unsigned char> vchSigScratch;
    opcodetype op;
    // For typical P2PKH scripts: <sig> <pubkey>
    if (!scriptSig.GetOp(it, op, vchSigScratch)) {
        return false;
    }
    std::vector<unsigned char> vchPubKey;
    if (!scriptSig.GetOp(it, op, vchPubKey)) {
        return false;
    }

    CPubKey pubkey(vchPubKey);
    if (!pubkey.IsValid()) {
        return false;
    }

    return pubkey.Verify(block.GetHash(), block.vchBlockSig);
}

// Basic stake kernel hash now incorporates the current stake modifier:
// H( stake_modifier || prevout.hash || prevout.n || nTimeBlockFrom || nTimeTx )
static uint256 ComputeKernelHash(const uint256& stake_modifier,
                                 const COutPoint& prevout,
                                 unsigned int nTimeBlockFrom,
                                 unsigned int nTimeTx)
{
    HashWriter ss;
    ss << stake_modifier;
    ss << prevout.hash;
    ss << prevout.n;
    ss << nTimeBlockFrom;
    ss << nTimeTx;
    return ss.GetHash();
}

arith_uint256 SaturatingMultiply(const arith_uint256& a, uint64_t b)
{
    if (a == arith_uint256{0} || b == 0) return arith_uint256{0};

    arith_uint256 bn_b{b};
    arith_uint256 max_val{~arith_uint256{0}};
    arith_uint256 limit = max_val / bn_b;
    if (a > limit) return max_val;

    arith_uint256 result{a};
    result *= bn_b;
    return result;
}

bool CheckStakeKernelHash(const CBlockIndex* pindexPrev,
                          unsigned int nBits,
                          uint256 hashBlockFrom,
                          unsigned int nTimeBlockFrom,
                          CAmount amount,
                          const COutPoint& prevout,
                          unsigned int nTimeTx,
                          node::StakeModifierManager& stake_modman,
                          uint256& hashProofOfStake,
                          bool fPrintProofOfStake,
                          const Consensus::Params& params)
{
    if (!pindexPrev) return false;
    if (nTimeTx <= nTimeBlockFrom) return false;                   // must move forward in time
    if ((nTimeTx & params.nStakeTimestampMask) != 0) return false; // enforce mask alignment

    // Derive target from nBits
    bool fNegative = false;
    bool fOverflow = false;
    arith_uint256 bnTarget;
    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);
    if (fNegative || fOverflow || bnTarget == 0) return false;

    // Amount scaling: target * amount with overflow handled by saturation
    arith_uint256 bnTargetWeight = SaturatingMultiply(bnTarget, amount);

    uint256 stake_modifier;
    if (params.nStakeModifierVersion >= 1) {
        stake_modifier = stake_modman.GetCurrentModifier();
    } else {
        stake_modifier = GetStakeModifier(pindexPrev, nTimeTx, params);
    }
    hashProofOfStake = ComputeKernelHash(stake_modifier, prevout, nTimeBlockFrom, nTimeTx);
    arith_uint256 bnHash = UintToArith256(hashProofOfStake);

    if (fPrintProofOfStake) {
        LogDebug(BCLog::STAKING, "CheckStakeKernelHash: hash=%s target=%s amt=%lld\n",
                 hashProofOfStake.ToString(), bnTargetWeight.ToString(), amount);
    }

    if (bnHash > bnTargetWeight) return false;
    return true;
}

CAmount GetProofOfStakeReward(int nHeight, CAmount nFees, int64_t coin_age_weight, const Consensus::Params& params)
{
    CAmount subsidy = GetBlockSubsidy(nHeight, params);
    int64_t weight = std::min<int64_t>(coin_age_weight, params.nStakeMaxAgeWeight);
    CAmount staking_reward = subsidy * weight / params.nStakeMinAge;
    CAmount validator_fee = nFees * 9 / 10;
    return staking_reward + validator_fee;
}

bool ContextualCheckProofOfStake(const CBlock& block,
                                 const CBlockIndex* pindexPrev,
                                 const CCoinsViewCache& view,
                                 const CChain& chain,
                                 const Consensus::Params& params)
{
    if (!pindexPrev) return false;
    if (!IsProofOfStake(block)) return false;

    const CTransaction& coinstake = *block.vtx[1];

    // Enforce block time == coinstake tx time (v3 convention)
    if (block.nTime != coinstake.nLockTime) return false;
    if ((block.nTime & params.nStakeTimestampMask) != 0) return false;

    // Check single coinstake only
    for (size_t i = 2; i < block.vtx.size(); ++i) {
        if (IsCoinStakeTx(*block.vtx[i])) return false; // multiple coinstakes
    }

    // Use first input as kernel (common approach)
    if (coinstake.vin.empty()) return false;
    const CTxIn& txin = coinstake.vin[0];
    const Coin& coin = view.AccessCoin(txin.prevout);
    if (coin.IsSpent()) return false;

    int spend_height = pindexPrev->nHeight + 1; // height of the coinstake block
    int minConf = params.nStakeMinConfirmations > 0 ? params.nStakeMinConfirmations : 80;

    int coin_height = coin.nHeight;
    if (coin_height <= 0 || coin_height > spend_height) return false;
    int depth = spend_height - coin_height;
    if (depth < minConf) return false;

    // Minimum age (time based) – approximate using ancestor median time past difference.
    const CBlockIndex* pindexFrom = chain[coin_height];
    if (!pindexFrom) return false;
    int64_t coin_age = block.GetBlockTime() - pindexFrom->GetBlockTime();
    if (coin_age < params.nStakeMinAge) return false; // enforce minimum stake age

    // Reconstruct previous stake source block time for kernel (using pindexFrom)
    unsigned int nTimeBlockFrom = pindexFrom->GetBlockTime();

    // Determine nBits / target for this PoS block and verify header difficulty
    unsigned int nBits = GetPoSNextTargetRequired(pindexPrev, block.GetBlockTime(), params);
    if (block.nBits != nBits) return false;

    uint256 hashProofOfStake;
    if (!CheckStakeKernelHash(pindexPrev, nBits,
                              pindexFrom->GetBlockHash(), nTimeBlockFrom,
                              coin.out.nValue, txin.prevout,
                              block.nTime, hashProofOfStake, false, params)) {
        return false;
    }

    // Fakestake mitigation (Blackcoin PoS v3.1 inspired): ensure inputs exist, are unique, and mature
    std::set<COutPoint> seen_outs;
    CAmount input_value = 0;
    for (const auto& in : coinstake.vin) {
        if (!seen_outs.insert(in.prevout).second) return false; // duplicate input
        const Coin& incoin = view.AccessCoin(in.prevout);
        if (incoin.IsSpent()) return false; // missing or already spent
        if (incoin.nHeight <= 0 || incoin.nHeight > spend_height) return false;
        int in_depth = spend_height - incoin.nHeight;
        if (in_depth < minConf) return false;
        input_value += incoin.out.nValue;
    }

    // Calculate total transaction fees from the rest of the block
    CAmount fees = 0;
    for (size_t i = 2; i < block.vtx.size(); ++i) {
        const CTransaction& tx = *block.vtx[i];
        CAmount in_val = 0;
        for (const auto& in : tx.vin) {
            const Coin& incoin = view.AccessCoin(in.prevout);
            if (incoin.IsSpent()) return false;
            in_val += incoin.out.nValue;
        }
        CAmount out_val = 0;
        for (const auto& o : tx.vout)
            out_val += o.nValue;
        if (in_val < out_val) return false;
        fees += in_val - out_val;
    }

    // Determine maximum allowed coinstake output value
    CAmount output_value = 0;
    for (const auto& o : coinstake.vout)
        output_value += o.nValue;

    CAmount reward = GetProofOfStakeReward(spend_height, fees, coin_age, params);
    CAmount max_allowed = input_value + reward;

    if (output_value < input_value) return false; // must not burn value
    if (output_value > max_allowed) return false; // cannot exceed allowed reward

    return true;
}