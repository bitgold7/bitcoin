#include <pow.h>

#include <arith_uint256.h>
#include <chain.h>
#include <consensus/params.h>

unsigned int GetPoSNextWorkRequired(const CBlockIndex* pindexLast, int64_t nBlockTime, const Consensus::Params& params)
{
    arith_uint256 bnLimit = UintToArith256(params.posLimit);
    if (pindexLast == nullptr) {
        return bnLimit.GetCompact();
    }

    int64_t target_spacing = 8 * 60; // 8 minutes
    int64_t interval = params.DifficultyAdjustmentInterval();
    int64_t actual_spacing = nBlockTime - pindexLast->GetBlockTime();
    if (actual_spacing < 0) actual_spacing = target_spacing;

    arith_uint256 bnNew;
    bnNew.SetCompact(pindexLast->nBits);
    bnNew *= ((interval - 1) * target_spacing + 2 * actual_spacing);
    bnNew /= ((interval + 1) * target_spacing);

    if (bnNew <= 0 || bnNew > bnLimit) {
        bnNew = bnLimit;
    }
    return bnNew.GetCompact();
}
