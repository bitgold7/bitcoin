#ifndef BITCOIN_POS_SLASHING_H
#define BITCOIN_POS_SLASHING_H

#include <set>
#include <string>
#include <utility>

namespace pos {

// Tracks validator evidence to detect double-signing events.
class SlashingTracker {
public:
    // Returns true if the validator produced more than one block at the given height.
    bool DetectDoubleSign(int height, const std::string& validator_id);

private:
    std::set<std::pair<int, std::string>> m_seen;
};

} // namespace pos

#endif // BITCOIN_POS_SLASHING_H
