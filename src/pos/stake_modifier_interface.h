#ifndef BITCOIN_POS_STAKE_MODIFIER_INTERFACE_H
#define BITCOIN_POS_STAKE_MODIFIER_INTERFACE_H

#include <uint256.h>

class CBlockIndex;
namespace Consensus { struct Params; }

/** Interface for providing stake modifier data to consensus checks. */
class StakeModifierProvider {
public:
    virtual ~StakeModifierProvider() = default;
    virtual uint256 GetCurrentModifier() const = 0;
    virtual uint256 GetDynamicModifier(const CBlockIndex* pindexPrev, unsigned int nTime,
                                       const Consensus::Params& params) = 0;
};

#endif // BITCOIN_POS_STAKE_MODIFIER_INTERFACE_H
