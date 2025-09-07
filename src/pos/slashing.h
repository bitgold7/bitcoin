#ifndef BITCOIN_POS_SLASHING_H
#define BITCOIN_POS_SLASHING_H

#include <cstdint>
#include <set>
#include <string>
#include <vector>

namespace pos {

// Reasons for which a validator can be slashed.
enum class SlashReason {
    DOUBLE_SIGN,
    NOTHING_AT_STAKE,
};

struct SlashingEvent {
    std::string validator_id;
    SlashReason reason;
    int64_t time;
};

// Tracks validator evidence to detect double-signing or nothing-at-stake events.
class SlashingTracker {
public:
    // Returns true if the validator was already seen (slash triggered) and records the event.
    bool AddEvidence(const std::string& validator_id, SlashReason reason, int64_t time);

    const std::vector<SlashingEvent>& GetEvents() const { return m_events; }

private:
    std::set<std::string> m_seen;
    std::vector<SlashingEvent> m_events;
};

// Global slashing tracker instance used by consensus and RPC layers.
extern SlashingTracker g_slashing_tracker;

// Convert a slashing reason to string.
const char* ToString(SlashReason reason);

} // namespace pos

#endif // BITCOIN_POS_SLASHING_H
