#include <boost/test/unit_test.hpp>

#ifdef ENABLE_BULLETPROOFS
extern "C" {
#include <secp256k1.h>
#include <secp256k1_generator.h>
#include <secp256k1_rangeproof.h>
}
#include <bulletproofs.h>
#include <random.h>

BOOST_AUTO_TEST_SUITE(bulletproof_tests)

BOOST_AUTO_TEST_CASE(verify_roundtrip)
{
    static secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
    uint64_t value = 5;
    unsigned char blind[32];
    unsigned char nonce[32];
    GetRandBytes(blind);
    GetRandBytes(nonce);
    secp256k1_pedersen_commitment commit;
    BOOST_CHECK(secp256k1_pedersen_commit(ctx, &commit, blind, value, secp256k1_generator_h));

    unsigned char proof_data[5134];
    size_t proof_len = sizeof(proof_data);
    BOOST_CHECK(secp256k1_rangeproof_sign(ctx, proof_data, &proof_len, 0, &commit, blind, nonce, 0, 64, value, secp256k1_generator_h));

    CBulletproof proof;
    proof.proof.assign(proof_data, proof_data + proof_len);
    proof.commitment.assign(reinterpret_cast<unsigned char*>(&commit),
                            reinterpret_cast<unsigned char*>(&commit) + sizeof(commit));

    BOOST_CHECK(VerifyBulletproof(proof));
}

BOOST_AUTO_TEST_SUITE_END()
#endif // ENABLE_BULLETPROOFS
