#include <node/stake_modifier_manager.h>

#include <algorithm>
#include <chain.h>
#include <chainparams.h>
#include <consensus/params.h>
#include <consensus/pos/stakemodifier.h>
#include <validation.h>
#include <vector>

namespace node {

StakeModifierManager::StakeModifierManager() : m_current_modifier{}, m_current_block_hash{} {}

std::optional<uint256> StakeModifierManager::GetModifier(const uint256& block_hash) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_modifiers.find(block_hash);
    if (it == m_modifiers.end()) return std::nullopt;
    return it->second.modifier;
}

uint256 StakeModifierManager::GetCurrentModifier() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_current_modifier;
}

uint256 StakeModifierManager::GetDynamicModifier(const CBlockIndex* pindexPrev, unsigned int nTime,
                                                 const Consensus::Params& params)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!pindexPrev) return m_current_modifier;
    auto it = m_modifiers.find(pindexPrev->GetBlockHash());
    if (it == m_modifiers.end()) return m_current_modifier;
    ModifierEntry& entry = it->second;
    if (static_cast<int64_t>(nTime) - entry.last_update >= params.nStakeModifierInterval) {
        entry.modifier = ComputeStakeModifier(pindexPrev, entry.modifier);
        entry.last_update = nTime;
        if (m_current_block_hash == pindexPrev->GetBlockHash()) {
            m_current_modifier = entry.modifier;
        }
    }
    return entry.modifier;
}

void StakeModifierManager::UpdateOnConnect(const CBlockIndex* pindex, const Consensus::Params& params)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    ModifierEntry entry{};
    if (pindex->pprev) {
        auto it = m_modifiers.find(pindex->pprev->GetBlockHash());
        if (it != m_modifiers.end()) entry = it->second;
    }
    if (entry.modifier.IsNull() || static_cast<int64_t>(pindex->nTime) - entry.last_update >= params.nStakeModifierInterval) {
        entry.modifier = ComputeStakeModifier(pindex->pprev, entry.modifier);
        entry.last_update = pindex->nTime;
    }
    m_current_modifier = entry.modifier;
    m_current_block_hash = pindex->GetBlockHash();
    m_modifiers[m_current_block_hash] = entry;
}

void StakeModifierManager::RemoveOnDisconnect(const CBlockIndex* pindex)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    const uint256 hash = pindex->GetBlockHash();
    m_modifiers.erase(hash);
    if (m_current_block_hash == hash) {
        if (pindex->pprev) {
            m_current_block_hash = pindex->pprev->GetBlockHash();
            auto it = m_modifiers.find(m_current_block_hash);
            if (it != m_modifiers.end()) {
                m_current_modifier = it->second.modifier;
            } else {
                m_current_modifier.SetNull();
            }
        } else {
            m_current_block_hash.SetNull();
            m_current_modifier.SetNull();
        }
    }
}

std::optional<uint256> StakeModifierManager::GetStakeModifier(ChainstateManager& chainman, const uint256& block_hash)
{
    LOCK(cs_main);
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_modifiers.find(block_hash);
    if (it != m_modifiers.end()) return it->second.modifier;

    const CBlockIndex* index = chainman.m_blockman.LookupBlockIndex(block_hash);
    if (index == nullptr) return std::nullopt;

    // Walk back until a cached modifier is found.
    std::vector<const CBlockIndex*> ancestors;
    uint256 modifier{};
    for (const CBlockIndex* p = index->pprev; p; p = p->pprev) {
        auto it2 = m_modifiers.find(p->GetBlockHash());
        if (it2 != m_modifiers.end()) {
            modifier = it2->second.modifier;
            break;
        }
        ancestors.push_back(p);
    }
    std::reverse(ancestors.begin(), ancestors.end());
    for (const CBlockIndex* ancestor : ancestors) {
        modifier = ComputeStakeModifier(ancestor, modifier);
        m_modifiers.emplace(ancestor->GetBlockHash(), ModifierEntry{modifier, ancestor->nTime});
    }
    modifier = ComputeStakeModifier(index->pprev, modifier);
    m_modifiers.emplace(block_hash, ModifierEntry{modifier, index->nTime});
    return modifier;
}

bool StakeModifierManager::ProcessStakeModifier(ChainstateManager& chainman, const uint256& block_hash, const uint256& modifier)
{
    LOCK(cs_main);
    std::lock_guard<std::mutex> lock(m_mutex);
    const CBlockIndex* index = chainman.m_blockman.LookupBlockIndex(block_hash);
    if (index == nullptr) return false;

    // Determine expected modifier using cached data.
    std::vector<const CBlockIndex*> ancestors;
    uint256 prev_mod{};
    for (const CBlockIndex* p = index->pprev; p; p = p->pprev) {
        auto it = m_modifiers.find(p->GetBlockHash());
        if (it != m_modifiers.end()) {
            prev_mod = it->second.modifier;
            break;
        }
        ancestors.push_back(p);
    }
    std::reverse(ancestors.begin(), ancestors.end());
    for (const CBlockIndex* ancestor : ancestors) {
        prev_mod = ComputeStakeModifier(ancestor, prev_mod);
        m_modifiers[ancestor->GetBlockHash()] = ModifierEntry{prev_mod, ancestor->nTime};
    }
    uint256 expected = ComputeStakeModifier(index->pprev, prev_mod);
    if (expected != modifier) return false;

    m_modifiers[block_hash] = ModifierEntry{modifier, index->nTime};
    return true;
}

void StakeModifierManager::BlockConnected(ChainstateRole role, const std::shared_ptr<const CBlock>&, const CBlockIndex* pindex)
{
    if (role == ChainstateRole::BACKGROUND) return;
    UpdateOnConnect(pindex, Params().GetConsensus());
}

void StakeModifierManager::BlockDisconnected(const std::shared_ptr<const CBlock>&, const CBlockIndex* pindex)
{
    RemoveOnDisconnect(pindex);
}

} // namespace node

