#ifndef BITCOIN_WALLET_STAKE_H
#define BITCOIN_WALLET_STAKE_H

class CBlockIndex;
namespace Consensus {
struct Params;
}

namespace wallet {

/**
 * Compute a staking timestamp for the next block.
 * Returns true and sets nTimeOut if the current node time permits staking.
 * Returns false if staking should be delayed because the required time
 * would violate the 15-second future drift rule.
 */
bool GetStakeTime(const CBlockIndex& prev, const Consensus::Params& params, unsigned int& nTimeOut);

} // namespace wallet

#endif // BITCOIN_WALLET_STAKE_H

