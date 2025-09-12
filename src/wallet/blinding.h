#ifndef BITCOIN_WALLET_BLINDING_H
#define BITCOIN_WALLET_BLINDING_H

#include <key.h>
#include <pubkey.h>
#include <map>

namespace wallet {
/** Simple manager for wallet blinding keys used to derive confidential addresses. */
class BlindingKeyManager {
private:
    std::map<CKeyID, CKey> m_keys;
public:
    /** Get the blinding key for a given public key, creating it if necessary. */
    CKey GetOrCreateBlindingKey(const CPubKey& pubkey);
    /** Return an existing blinding key for a public key, if available. */
    CKey GetBlindingKey(const CPubKey& pubkey) const;
};
} // namespace wallet

#endif // BITCOIN_WALLET_BLINDING_H
