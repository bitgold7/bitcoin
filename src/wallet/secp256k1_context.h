#ifndef BITCOIN_WALLET_SECP256K1_CONTEXT_H
#define BITCOIN_WALLET_SECP256K1_CONTEXT_H

#include <secp256k1.h>

class secp256k1_context_holder {
    secp256k1_context* m_ctx;

public:
    explicit secp256k1_context_holder(unsigned int flags) noexcept
        : m_ctx(secp256k1_context_create(flags)) {}
    ~secp256k1_context_holder() { secp256k1_context_destroy(m_ctx); }

    secp256k1_context_holder(const secp256k1_context_holder&) = delete;
    secp256k1_context_holder& operator=(const secp256k1_context_holder&) = delete;

    secp256k1_context* get() const { return m_ctx; }
};

#endif // BITCOIN_WALLET_SECP256K1_CONTEXT_H
