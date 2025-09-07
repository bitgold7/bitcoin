#include "pos/validator.h"
#include "pos/stake.h"

namespace pos {

Validator::Validator(uint64_t stake_amount)
    : m_stake_amount(stake_amount),
      m_locked_until(0),
      m_active(false) {}

void Validator::Activate(int64_t current_time)
{
    current_time &= ~STAKE_TIMESTAMP_MASK;
    if (m_stake_amount >= MIN_STAKE && current_time >= m_locked_until) {
        m_active = true;
    }
}

void Validator::ScheduleUnstake(int64_t current_time)
{
    current_time &= ~STAKE_TIMESTAMP_MASK;
    m_locked_until = current_time + UNSTAKE_DELAY;
    m_active = false;
}

void Validator::Slash(SlashType reason)
{
    uint8_t penalty = reason == SlashType::DOUBLE_SIGN ? SLASH_PENALTY_DOUBLE_SIGN : SLASH_PENALTY_NOTHING_AT_STAKE;
    const uint64_t amount = m_stake_amount * penalty / 100;
    if (amount >= m_stake_amount) {
        m_stake_amount = 0;
    } else {
        m_stake_amount -= amount;
    }
    m_active = false;
    m_locked_until = 0;
}

} // namespace pos
