#ifndef BITCOIN_NODE_STAKE_MODIFIER_MANAGER_H
#define BITCOIN_NODE_STAKE_MODIFIER_MANAGER_H

#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <uint256.h>
#include <validationinterface.h>

class CBlockIndex;
class ChainstateManager;
namespace Consensus { struct Params; }

namespace node {

/**
 * Manages cached stake modifiers for consensus and networking.
 */
class StakeModifierManager : public CValidationInterface
{
private:
    struct ModifierEntry {
        uint256 modifier;
        int64_t last_update{0};
    };

    mutable std::mutex m_mutex;
    std::map<uint256, ModifierEntry> m_modifiers;
    uint256 m_current_modifier;
    uint256 m_current_block_hash;

public:
    StakeModifierManager();

    /** Return the modifier for a block if cached. */
    std::optional<uint256> GetModifier(const uint256& block_hash) const;

    /** Return the current stake modifier. */
    uint256 GetCurrentModifier() const;

    /**
     * Return the current stake modifier, refreshing it if the provided time
     * exceeds the refresh interval (v3 algorithm).
     */
    uint256 GetDynamicModifier(const CBlockIndex* pindexPrev, unsigned int nTime,
                               const Consensus::Params& params);

    /** Update the modifier on block connect. */
    void UpdateOnConnect(const CBlockIndex* pindex, const Consensus::Params& params);

    /** Remove the modifier on block disconnect. */
    void RemoveOnDisconnect(const CBlockIndex* pindex);

    /** Retrieve modifier for a block, computing ancestors if necessary. */
    std::optional<uint256> GetStakeModifier(ChainstateManager& chainman, const uint256& block_hash);

    /** Validate and store a received stake modifier message. */
    bool ProcessStakeModifier(ChainstateManager& chainman, const uint256& block_hash, const uint256& modifier);

    void BlockConnected(ChainstateRole role, const std::shared_ptr<const CBlock>& block, const CBlockIndex* pindex) override;
    void BlockDisconnected(const std::shared_ptr<const CBlock>& block, const CBlockIndex* pindex) override;
};

} // namespace node

#endif // BITCOIN_NODE_STAKE_MODIFIER_MANAGER_H
