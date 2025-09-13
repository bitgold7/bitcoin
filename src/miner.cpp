#include <miner.h>
#include <validation.h>

/**
 * Legacy proof-of-work mining is disabled after the genesis block.
 * This placeholder guards any accidental calls into mining routines.
 */
bool MineBlock(const CChain& chain, CBlock& block)
{
    if (chain.Height() >= 0) {
        // PoW disabled once a chain exists beyond genesis.
        return false;
    }
    return false;
}
