#include <test/util/setup_common.h>

#include <bulletproofs.h>
#include <bulletproofs_utils.h>
#include <consensus/amount.h>
#include <hash.h>
#include <primitives/transaction.h>
#include <script/script.h>

#include <array>
#include <span>

BOOST_FIXTURE_TEST_SUITE(bulletproof_tests, BasicTestingSetup)

static CTransaction MakeTestTransaction()
{
    CMutableTransaction mtx;
    mtx.vin.emplace_back();
    mtx.vout.emplace_back(CAmount{1 * COIN}, CScript() << OP_TRUE);
    mtx.vout.emplace_back(CAmount{2 * COIN}, CScript() << OP_TRUE);
    return CTransaction{mtx};
}

BOOST_AUTO_TEST_CASE(verify_bulletproof_success)
{
#ifdef ENABLE_BULLETPROOFS
    const CTransaction tx = MakeTestTransaction();
    CBulletproof proof;
    PopulateBulletproofProof(tx, proof);
    const auto commitments = ComputeBulletproofCommitments(tx);
    BOOST_CHECK_EQUAL(proof.commitments, commitments);
    BOOST_CHECK(VerifyBulletproof(proof, commitments));
#else
    CBulletproof proof;
    BOOST_CHECK(!VerifyBulletproof(proof, std::span<const std::vector<unsigned char>>{}));
#endif
}

#ifdef ENABLE_BULLETPROOFS
BOOST_AUTO_TEST_CASE(verify_bulletproof_reject_tampered)
{
    const CTransaction tx = MakeTestTransaction();
    CBulletproof proof;
    PopulateBulletproofProof(tx, proof);
    auto commitments = ComputeBulletproofCommitments(tx);

    CBulletproof tampered_proof = proof;
    tampered_proof.proof[0] ^= 0x01;
    BOOST_CHECK(!VerifyBulletproof(tampered_proof, commitments));

    tampered_proof = proof;
    tampered_proof.commitments[0][0] ^= 0x01;
    BOOST_CHECK(!VerifyBulletproof(tampered_proof, commitments));
}
#endif

BOOST_AUTO_TEST_SUITE_END()
