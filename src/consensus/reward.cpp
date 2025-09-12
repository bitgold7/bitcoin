#include <consensus/reward.h>

#include <consensus/validation.h>
#include <dividend/dividend.h>
#include <primitives/transaction.h>

namespace Consensus {

bool CheckRewardSplit(const CTransaction& reward_tx,
                      CAmount stake_input_total,
                      CAmount validator_reward,
                      CAmount dividend_reward,
                      BlockValidationState& state)
{
    if (reward_tx.vout.size() < 2) {
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS,
                             "bad-validator-amount");
    }
    if (reward_tx.vout.size() < 3) {
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS,
                             "bad-dividend-missing");
    }

    const CTxOut& dividend_out{reward_tx.vout.back()};
    if (dividend_out.nValue != dividend_reward) {
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS,
                             "bad-dividend-amount");
    }
    if (dividend_out.scriptPubKey != dividend::GetDividendScript()) {
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS,
                             "bad-dividend-script");
    }

    CAmount total{0};
    for (size_t i = 1; i + 1 < reward_tx.vout.size(); ++i) {
        total += reward_tx.vout[i].nValue;
    }
    if (total != stake_input_total + validator_reward) {
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS,
                             "bad-validator-amount");
    }

    return true;
}

} // namespace Consensus

