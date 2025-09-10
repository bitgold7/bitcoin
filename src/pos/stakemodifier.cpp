#include <pos/stakemodifier.h>

#include <chain.h>
#include <hash.h>

uint256 ComputeStakeModifier(const CBlockIndex* pindexPrev, const uint256& prev_modifier)
{
    HashWriter ss;
    ss << prev_modifier;
    if (pindexPrev) {
        ss << pindexPrev->GetBlockHash() << pindexPrev->nHeight << pindexPrev->nTime;
    }
    return ss.GetHash();
}
