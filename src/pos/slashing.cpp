#include "pos/slashing.h"

namespace pos {

SlashingTracker g_slashing_tracker;

bool SlashingTracker::AddEvidence(const std::string& validator_id, SlashReason reason, int64_t time)
{
    const bool duplicate = m_seen.count(validator_id);
    if (duplicate) {
        m_events.push_back({validator_id, reason, time});
    } else {
        m_seen.insert(validator_id);
    }
    return duplicate;
}

const char* ToString(SlashReason reason)
{
    switch (reason) {
    case SlashReason::DOUBLE_SIGN:
        return "double-sign";
    case SlashReason::NOTHING_AT_STAKE:
        return "nothing-at-stake";
    }
    return "unknown";
}

} // namespace pos
