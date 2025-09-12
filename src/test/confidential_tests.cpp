#include <test/util/setup_common.h>
#include <boost/test/unit_test.hpp>

#ifdef ENABLE_BULLETPROOFS
#include <bulletproofs.h>
#include <coins.h>
#include <consensus/tx_verify.h>
#include <random.h>
#include <util/secp256k1_context.h>
#include <uint256.h>
#endif

BOOST_FIXTURE_TEST_SUITE(confidential_tests, BasicTestingSetup)

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

BOOST_AUTO_TEST_CASE(tampered_proof_fails)
{
    CCoinsViewDummy dummy;
    CCoinsViewCache view(&dummy);

    // Funding transaction with a Bulletproof output.
    CMutableTransaction fund;
    fund.version = CTransaction::CURRENT_VERSION | CTransaction::BULLETPROOF_VERSION;
    fund.vin.emplace_back(COutPoint(uint256::ONE, 0), CScript(), 0);
    CBulletproof bp_in = MakeBulletproof(/*value=*/5);
    CScript out_script;
    out_script << OP_BULLETPROOF
               << std::vector<unsigned char>(bp_in.commitment.data,
                                             bp_in.commitment.data + sizeof(bp_in.commitment.data))
               << bp_in.proof;
    fund.vout.emplace_back(CAmount{0}, out_script);
    const CTransaction fund_tx{fund};
    AddCoins(view, fund_tx, /*nHeight=*/1);

    // Spending transaction.
    CMutableTransaction spend;
    spend.version = CTransaction::CURRENT_VERSION | CTransaction::BULLETPROOF_VERSION;
    spend.vin.emplace_back(COutPoint(fund_tx.GetHash(), 0), out_script, 0);
    CBulletproof bp_out = MakeBulletproof(/*value=*/5);
    CScript spend_out_script;
    spend_out_script << OP_BULLETPROOF
                     << std::vector<unsigned char>(bp_out.commitment.data,
                                                   bp_out.commitment.data + sizeof(bp_out.commitment.data))
                     << bp_out.proof;
    spend.vout.emplace_back(CAmount{0}, spend_out_script);
    const CTransaction tx_valid{spend};
    TxValidationState state;
    CAmount fee;
    BOOST_CHECK(Consensus::CheckTxInputs(tx_valid, state, view, /*nSpendHeight=*/1, fee));

    // Tamper with the proof and ensure verification fails.
    std::vector<unsigned char> bad_proof = bp_out.proof;
    if (!bad_proof.empty()) bad_proof[0] ^= 1;
    CScript bad_out_script;
    bad_out_script << OP_BULLETPROOF
                   << std::vector<unsigned char>(bp_out.commitment.data,
                                                 bp_out.commitment.data + sizeof(bp_out.commitment.data))
                   << bad_proof;
    CMutableTransaction tampered = spend;
    tampered.vout[0].scriptPubKey = bad_out_script;
    const CTransaction tx_bad{tampered};
    TxValidationState state_bad;
    BOOST_CHECK(!Consensus::CheckTxInputs(tx_bad, state_bad, view, 1, fee));
}

BOOST_AUTO_TEST_CASE(mixed_transparent_confidential_sums)
{
    CCoinsViewDummy dummy;
    CCoinsViewCache view(&dummy);

    // Bulletproof funding transaction (value 5).
    CMutableTransaction fund_bp;
    fund_bp.version = CTransaction::CURRENT_VERSION | CTransaction::BULLETPROOF_VERSION;
    fund_bp.vin.emplace_back(COutPoint(uint256::ONE, 1), CScript(), 0);
    CBulletproof bp = MakeBulletproof(/*value=*/5);
    CScript bp_script;
    bp_script << OP_BULLETPROOF
              << std::vector<unsigned char>(bp.commitment.data,
                                            bp.commitment.data + sizeof(bp.commitment.data))
              << bp.proof;
    fund_bp.vout.emplace_back(CAmount{0}, bp_script);
    const CTransaction fund_bp_tx{fund_bp};
    AddCoins(view, fund_bp_tx, 1);

    // Transparent funding transaction (value 3).
    CMutableTransaction fund_tr;
    fund_tr.vin.emplace_back(COutPoint(uint256::ONE, 2), CScript(), 0);
    fund_tr.vout.emplace_back(CAmount{3}, CScript());
    const CTransaction fund_tr_tx{fund_tr};
    AddCoins(view, fund_tr_tx, 1);

    // Spending transaction mixing both types.
    CMutableTransaction spend;
    spend.version = CTransaction::CURRENT_VERSION | CTransaction::BULLETPROOF_VERSION;
    // Bulletproof input.
    spend.vin.emplace_back(COutPoint(fund_bp_tx.GetHash(), 0), bp_script, 0);
    // Transparent input.
    spend.vin.emplace_back(COutPoint(fund_tr_tx.GetHash(), 0), CScript(), 0);
    // Bulletproof output value 5.
    CBulletproof bp_out = MakeBulletproof(/*value=*/5);
    CScript bp_out_script;
    bp_out_script << OP_BULLETPROOF
                  << std::vector<unsigned char>(bp_out.commitment.data,
                                                bp_out.commitment.data + sizeof(bp_out.commitment.data))
                  << bp_out.proof;
    spend.vout.emplace_back(CAmount{0}, bp_out_script);
    // Transparent output value 2.
    spend.vout.emplace_back(CAmount{2}, CScript());

    const CTransaction tx{spend};
    TxValidationState state;
    CAmount fee;
    BOOST_CHECK(Consensus::CheckTxInputs(tx, state, view, 1, fee));
    BOOST_CHECK_EQUAL(fee, 1);
}

#else
BOOST_AUTO_TEST_CASE(dummy_confidential_tests)
{
    BOOST_TEST_MESSAGE("Bulletproofs disabled, skipping confidential tests");
}
#endif

BOOST_AUTO_TEST_SUITE_END()

