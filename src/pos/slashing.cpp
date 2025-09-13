#include "pos/slashing.h"

namespace pos {

bool SlashingTracker::DetectDoubleSign(int height, const std::string& validator_id)
{
    auto key = std::make_pair(height, validator_id);
    if (m_seen.count(key)) {
        return true;
    }
    m_seen.insert(key);
    return false;
}

} // namespace pos
