#include <wallet/blinding.h>

namespace wallet {

CKey BlindingKeyManager::GetOrCreateBlindingKey(const CPubKey& pubkey)
{
    CKeyID id = pubkey.GetID();
    auto it = m_keys.find(id);
    if (it != m_keys.end()) return it->second;
    CKey key;
    key.MakeNewKey(true);
    m_keys.emplace(id, key);
    return key;
}

CKey BlindingKeyManager::GetBlindingKey(const CPubKey& pubkey) const
{
    CKeyID id = pubkey.GetID();
    auto it = m_keys.find(id);
    if (it != m_keys.end()) return it->second;
    return CKey();
}

} // namespace wallet
