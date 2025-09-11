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
#endif

struct CBulletproof {
    std::vector<unsigned char> proof;
    std::vector<unsigned char> commitment; // serialized secp256k1_pedersen_commitment
};

inline bool VerifyBulletproof(const CBulletproof& proof)
{
#ifdef ENABLE_BULLETPROOFS
    if (proof.proof.empty() || proof.commitment.size() != sizeof(secp256k1_pedersen_commitment)) {
        return false;
    }

    static secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY);

    secp256k1_pedersen_commitment commit;
    std::memcpy(&commit, proof.commitment.data(), sizeof(commit));

    uint64_t min_value = 0, max_value = 0;
    int ret = secp256k1_rangeproof_verify(ctx, &min_value, &max_value, &commit,
                                          proof.proof.data(), proof.proof.size(),
                                          nullptr, 0, secp256k1_generator_h);
    return ret == 1;
#else
    (void)proof;
    return false;
#endif
}

#endif // BITCOIN_BULLETPROOFS_H
