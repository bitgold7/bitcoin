#include <test/util/setup_common.h>
#include <boost/test/unit_test.hpp>

#ifdef ENABLE_BULLETPROOFS
#include <bulletproofs.h>
#include <random.h>
#include <util/secp256k1_context.h>
#endif

BOOST_FIXTURE_TEST_SUITE(bulletproof_tests, BasicTestingSetup)

#ifdef ENABLE_BULLETPROOFS
static CBulletproof MakeBulletproof(CAmount value)
{
    static const secp256k1_context_holder ctx(SECP256K1_CONTEXT_SIGN);
    CBulletproof bp;
    unsigned char blind[32];
    GetRandBytes({blind, 32});
    BOOST_REQUIRE(secp256k1_pedersen_commit(ctx.get(), &bp.commitment, blind, value, &secp256k1_generator_h) == 1);
    bp.proof.resize(SECP256K1_RANGE_PROOF_MAX_LENGTH);
    size_t proof_len = bp.proof.size();
    BOOST_REQUIRE(secp256k1_rangeproof_sign(ctx.get(), bp.proof.data(), &proof_len, 0, &bp.commitment, blind,
                                           nullptr, 0, 0, value, &secp256k1_generator_h) == 1);
    bp.proof.resize(proof_len);
    bp.extra.assign(blind, blind + 32);
    return bp;
}
#endif

BOOST_AUTO_TEST_CASE(verify_bulletproof)
{
#ifdef ENABLE_BULLETPROOFS
    CBulletproof bp = MakeBulletproof(/*value=*/0);
    for (int i = 0; i < 10; ++i) {
        BOOST_CHECK(VerifyBulletproof(bp));
    }
    CBulletproof bad = bp;
    bad.proof[0] ^= 1;
    for (int i = 0; i < 10; ++i) {
        BOOST_CHECK(!VerifyBulletproof(bad));
    }
#else
    BOOST_TEST_MESSAGE("Bulletproofs disabled, skipping test");
#endif
}

BOOST_AUTO_TEST_SUITE_END()
