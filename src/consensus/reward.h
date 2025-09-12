#ifndef BITCOIN_CONSENSUS_REWARD_H
#define BITCOIN_CONSENSUS_REWARD_H

#include <consensus/amount.h>

class CTransaction;
class BlockValidationState;

namespace Consensus {

/**
 * Validate the validator/dividend reward split for a block's reward
 * transaction. The transaction is expected to pay the combined stake
 * inputs and validator reward to one or more outputs, followed by a
 * final dividend output with the provided amount and standard
 * dividend script.
 */
bool CheckRewardSplit(const CTransaction& reward_tx,
                      CAmount stake_input_total,
                      CAmount validator_reward,
                      CAmount dividend_reward,
                      BlockValidationState& state);

} // namespace Consensus

#endif // BITCOIN_CONSENSUS_REWARD_H

