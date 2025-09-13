#include <wallet/stake.h>

#include <chain.h>
#include <util/time.h>

namespace wallet {

bool GetStakeTime(const CBlockIndex& prev, const Consensus::Params& params, unsigned int& nTimeOut)
{
    const int64_t now{GetTime()};
    int64_t nTime{std::max<int64_t>(prev.GetBlockTime() + params.nStakeTargetSpacing, now)};
    nTime = (nTime + params.nStakeTimestampMask) & ~params.nStakeTimestampMask;
    if (nTime > now + 15) return false;
    nTimeOut = nTime;
    return true;
}

} // namespace wallet

