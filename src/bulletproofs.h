#ifndef BITCOIN_BULLETPROOFS_H
#define BITCOIN_BULLETPROOFS_H

#include <vector>

#ifdef ENABLE_BULLETPROOFS
extern "C" {
#include <secp256k1.h>
#include <secp256k1_generator.h>
#include <secp256k1_rangeproof.h>
}
#include <cstring>
#include <serialize.h>
#endif

struct CBulletproof {
#ifdef ENABLE_BULLETPROOFS
    secp256k1_pedersen_commitment commitment{};
#endif
    std::vector<unsigned char> proof;
#ifdef ENABLE_BULLETPROOFS
    std::vector<unsigned char> extra;

    template <typename Stream>
    void Serialize(Stream& s) const
    {
        s << std::span{commitment.data, sizeof(commitment.data)}
          << proof
          << extra;
    }

    template <typename Stream>
    void Unserialize(Stream& s)
    {
        s.read(MakeWritableByteSpan(commitment.data));
        s >> proof;
        s >> extra;
    }
#endif
};

inline bool VerifyBulletproof(const CBulletproof& proof)
{
#ifdef ENABLE_BULLETPROOFS
    if (proof.proof.empty() || proof.proof.size() > SECP256K1_RANGE_PROOF_MAX_LENGTH) return false;

    static secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);

    uint64_t min_value = 0, max_value = 0;
    const unsigned char* extra = proof.extra.empty() ? nullptr : proof.extra.data();
    int ret = secp256k1_rangeproof_verify(ctx, &min_value, &max_value, &proof.commitment,
                                          proof.proof.data(), proof.proof.size(),
                                          extra, proof.extra.size(), secp256k1_generator_h);
    if (ret != 1) return false;
    if (min_value > max_value) return false;
    return true;
#else
    (void)proof;
    return false;
#endif
}

#endif // BITCOIN_BULLETPROOFS_H
