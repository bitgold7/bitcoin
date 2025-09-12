// Copyright (c) 2014-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <chain.h>
#include <consensus/amount.h>
#include <consensus/merkle.h>
#include <core_io.h>
#include <pow.h>
#include <hash.h>
#include <net.h>
#include <signet.h>
#include <uint256.h>
#include <util/chaintype.h>
#include <util/time.h>
#include <validation.h>
#include <script/script.h>
#include <arith_uint256.h>

#ifdef ENABLE_BULLETPROOFS
#include <bulletproofs.h>
#include <random.h>
#endif

#include <string>
#include <limits>

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(validation_tests, TestingSetup)

static void TestBlockSubsidyHalvings(const Consensus::Params& consensusParams)
{
    const CAmount genesis_reward{3'000'000 * COIN};
    BOOST_CHECK_EQUAL(GetBlockSubsidy(1, consensusParams), genesis_reward);

    const CAmount nInitialSubsidy{50 * COIN};
    BOOST_CHECK_EQUAL(GetBlockSubsidy(2, consensusParams), nInitialSubsidy);
    BOOST_CHECK_EQUAL(GetBlockSubsidy(consensusParams.nSubsidyHalvingInterval + 1, consensusParams), nInitialSubsidy);

    const int maxHalvings{64};
    CAmount nPreviousSubsidy{nInitialSubsidy};
    for (int nHalvings = 1; nHalvings < maxHalvings; ++nHalvings) {
        const int nHeight{2 + nHalvings * consensusParams.nSubsidyHalvingInterval};
        BOOST_CHECK_EQUAL(GetBlockSubsidy(nHeight - 1, consensusParams), nPreviousSubsidy);
        const CAmount nSubsidy{GetBlockSubsidy(nHeight, consensusParams)};
        BOOST_CHECK(nSubsidy <= nInitialSubsidy);
        BOOST_CHECK_EQUAL(nSubsidy, nPreviousSubsidy / 2);
        nPreviousSubsidy = nSubsidy;
    }
    BOOST_CHECK_EQUAL(GetBlockSubsidy(2 + maxHalvings * consensusParams.nSubsidyHalvingInterval, consensusParams), 0);
}

static void TestBlockSubsidyHalvings(int nSubsidyHalvingInterval)
{
    Consensus::Params consensusParams;
    consensusParams.nSubsidyHalvingInterval = nSubsidyHalvingInterval;
    consensusParams.nMaximumSupply = std::numeric_limits<CAmount>::max();
    consensusParams.genesis_reward = 0;
    TestBlockSubsidyHalvings(consensusParams);
}

BOOST_AUTO_TEST_CASE(block_subsidy_test)
{
    const auto chainParams = CreateChainParams(*m_node.args, ChainType::MAIN);
    TestBlockSubsidyHalvings(chainParams->GetConsensus()); // As in main
    TestBlockSubsidyHalvings(150); // Test a shorter interval
    TestBlockSubsidyHalvings(1000); // Just another interval
}

BOOST_AUTO_TEST_CASE(subsidy_limit_test)
{
    const auto chainParams = CreateChainParams(*m_node.args, ChainType::MAIN);
    const CAmount genesis_reward = GetBlockSubsidy(1, chainParams->GetConsensus());
    CAmount nSum = genesis_reward;
    for (int nHeight = 2; nHeight < 14000000; nHeight += 1000) {
        CAmount nSubsidy = GetBlockSubsidy(nHeight, chainParams->GetConsensus());
        BOOST_CHECK(nSubsidy <= 50 * COIN);
        nSum += nSubsidy * 1000;
        BOOST_CHECK(MoneyRange(nSum));
    }
    BOOST_CHECK_EQUAL(nSum, CAmount{8'000'000 * COIN});
}

BOOST_AUTO_TEST_CASE(cumulative_subsidy_never_exceeds_limit)
{
    const auto chainParams = CreateChainParams(*m_node.args, ChainType::MAIN);
    const auto& consensus{chainParams->GetConsensus()};
    const CAmount genesis_reward{GetBlockSubsidy(1, consensus)};
    CAmount cumulative{0};
    for (CAmount subsidy{GetBlockSubsidy(2, consensus)}; subsidy > 0; subsidy >>= 1) {
        cumulative += subsidy * consensus.nSubsidyHalvingInterval;
        BOOST_CHECK_LE(cumulative + genesis_reward, CAmount{8'000'000 * COIN});
    }
}

BOOST_AUTO_TEST_CASE(signet_parse_tests)
{
    ArgsManager signet_argsman;
    signet_argsman.ForceSetArg("-signetchallenge", "51"); // set challenge to OP_TRUE
    const auto signet_params = CreateChainParams(signet_argsman, ChainType::SIGNET);
    CBlock block;
    BOOST_CHECK(signet_params->GetConsensus().signet_challenge == std::vector<uint8_t>{OP_TRUE});
    CScript challenge{OP_TRUE};

    // empty block is invalid
    BOOST_CHECK(!SignetTxs::Create(block, challenge));
    BOOST_CHECK(!CheckSignetBlockSolution(block, signet_params->GetConsensus()));

    // no witness commitment
    CMutableTransaction cb;
    cb.vout.emplace_back(0, CScript{});
    block.vtx.push_back(MakeTransactionRef(cb));
    block.vtx.push_back(MakeTransactionRef(cb)); // Add dummy tx to exercise merkle root code
    BOOST_CHECK(!SignetTxs::Create(block, challenge));
    BOOST_CHECK(!CheckSignetBlockSolution(block, signet_params->GetConsensus()));

    // no header is treated valid
    std::vector<uint8_t> witness_commitment_section_141{0xaa, 0x21, 0xa9, 0xed};
    for (int i = 0; i < 32; ++i) {
        witness_commitment_section_141.push_back(0xff);
    }
    cb.vout.at(0).scriptPubKey = CScript{} << OP_RETURN << witness_commitment_section_141;
    block.vtx.at(0) = MakeTransactionRef(cb);
    BOOST_CHECK(SignetTxs::Create(block, challenge));
    BOOST_CHECK(CheckSignetBlockSolution(block, signet_params->GetConsensus()));

    // no data after header, valid
    std::vector<uint8_t> witness_commitment_section_325{0xec, 0xc7, 0xda, 0xa2};
    cb.vout.at(0).scriptPubKey = CScript{} << OP_RETURN << witness_commitment_section_141 << witness_commitment_section_325;
    block.vtx.at(0) = MakeTransactionRef(cb);
    BOOST_CHECK(SignetTxs::Create(block, challenge));
    BOOST_CHECK(CheckSignetBlockSolution(block, signet_params->GetConsensus()));

    // Premature end of data, invalid
    witness_commitment_section_325.push_back(0x01);
    witness_commitment_section_325.push_back(0x51);
    cb.vout.at(0).scriptPubKey = CScript{} << OP_RETURN << witness_commitment_section_141 << witness_commitment_section_325;
    block.vtx.at(0) = MakeTransactionRef(cb);
    BOOST_CHECK(!SignetTxs::Create(block, challenge));
    BOOST_CHECK(!CheckSignetBlockSolution(block, signet_params->GetConsensus()));

    // has data, valid
    witness_commitment_section_325.push_back(0x00);
    cb.vout.at(0).scriptPubKey = CScript{} << OP_RETURN << witness_commitment_section_141 << witness_commitment_section_325;
    block.vtx.at(0) = MakeTransactionRef(cb);
    BOOST_CHECK(SignetTxs::Create(block, challenge));
    BOOST_CHECK(CheckSignetBlockSolution(block, signet_params->GetConsensus()));

    // Extraneous data, invalid
    witness_commitment_section_325.push_back(0x00);
    cb.vout.at(0).scriptPubKey = CScript{} << OP_RETURN << witness_commitment_section_141 << witness_commitment_section_325;
    block.vtx.at(0) = MakeTransactionRef(cb);
    BOOST_CHECK(!SignetTxs::Create(block, challenge));
    BOOST_CHECK(!CheckSignetBlockSolution(block, signet_params->GetConsensus()));
}

//! Test retrieval of valid assumeutxo values.
BOOST_AUTO_TEST_CASE(test_assumeutxo)
{
    const auto params = CreateChainParams(*m_node.args, ChainType::REGTEST);

    // These heights don't have assumeutxo configurations associated, per the contents
    // of kernel/chainparams.cpp.
    std::vector<int> bad_heights{0, 100, 111, 115, 209, 211};

    for (auto empty : bad_heights) {
        const auto out = params->AssumeutxoForHeight(empty);
        BOOST_CHECK(!out);
    }

    const auto out110 = *params->AssumeutxoForHeight(110);
    BOOST_CHECK_EQUAL(out110.hash_serialized.ToString(), "b952555c8ab81fec46f3d4253b7af256d766ceb39fb7752b9d18cdf4a0141327");
    BOOST_CHECK_EQUAL(out110.m_chain_tx_count, 111U);

    const auto out110_2 = *params->AssumeutxoForBlockhash(uint256{"6affe030b7965ab538f820a56ef56c8149b7dc1d1c144af57113be080db7c397"});
    BOOST_CHECK_EQUAL(out110_2.hash_serialized.ToString(), "b952555c8ab81fec46f3d4253b7af256d766ceb39fb7752b9d18cdf4a0141327");
    BOOST_CHECK_EQUAL(out110_2.m_chain_tx_count, 111U);
}

BOOST_AUTO_TEST_CASE(block_malleation)
{
    // Test utilities that calls `IsBlockMutated` and then clears the validity
    // cache flags on `CBlock`.
    auto is_mutated = [](CBlock& block, bool check_witness_root) {
        bool mutated{IsBlockMutated(block, check_witness_root)};
        block.fChecked = false;
        block.m_checked_witness_commitment = false;
        block.m_checked_merkle_root = false;
        return mutated;
    };
    auto is_not_mutated = [&is_mutated](CBlock& block, bool check_witness_root) {
        return !is_mutated(block, check_witness_root);
    };

    // Test utilities to create coinbase transactions and insert witness
    // commitments.
    //
    // Note: this will not include the witness stack by default to avoid
    // triggering the "no witnesses allowed for blocks that don't commit to
    // witnesses" rule when testing other malleation vectors.
    auto create_coinbase_tx = [](bool include_witness = false) {
        CMutableTransaction coinbase;
        coinbase.vin.resize(1);
        if (include_witness) {
            coinbase.vin[0].scriptWitness.stack.resize(1);
            coinbase.vin[0].scriptWitness.stack[0] = std::vector<unsigned char>(32, 0x00);
        }

        coinbase.vout.resize(1);
        coinbase.vout[0].scriptPubKey.resize(MINIMUM_WITNESS_COMMITMENT);
        coinbase.vout[0].scriptPubKey[0] = OP_RETURN;
        coinbase.vout[0].scriptPubKey[1] = 0x24;
        coinbase.vout[0].scriptPubKey[2] = 0xaa;
        coinbase.vout[0].scriptPubKey[3] = 0x21;
        coinbase.vout[0].scriptPubKey[4] = 0xa9;
        coinbase.vout[0].scriptPubKey[5] = 0xed;

        auto tx = MakeTransactionRef(coinbase);
        assert(tx->IsCoinBase());
        return tx;
    };
    auto insert_witness_commitment = [](CBlock& block, uint256 commitment) {
        assert(!block.vtx.empty() && block.vtx[0]->IsCoinBase() && !block.vtx[0]->vout.empty());

        CMutableTransaction mtx{*block.vtx[0]};
        CHash256().Write(commitment).Write(std::vector<unsigned char>(32, 0x00)).Finalize(commitment);
        memcpy(&mtx.vout[0].scriptPubKey[6], commitment.begin(), 32);
        block.vtx[0] = MakeTransactionRef(mtx);
    };

    {
        CBlock block;

        // Empty block is expected to have merkle root of 0x0.
        BOOST_CHECK(block.vtx.empty());
        block.hashMerkleRoot = uint256{1};
        BOOST_CHECK(is_mutated(block, /*check_witness_root=*/false));
        block.hashMerkleRoot = uint256{};
        BOOST_CHECK(is_not_mutated(block, /*check_witness_root=*/false));

        // Block with a single coinbase tx is mutated if the merkle root is not
        // equal to the coinbase tx's hash.
        block.vtx.push_back(create_coinbase_tx());
        BOOST_CHECK(block.vtx[0]->GetHash() != block.hashMerkleRoot);
        BOOST_CHECK(is_mutated(block, /*check_witness_root=*/false));
        block.hashMerkleRoot = block.vtx[0]->GetHash();
        BOOST_CHECK(is_not_mutated(block, /*check_witness_root=*/false));

        // Block with two transactions is mutated if the merkle root does not
        // match the double sha256 of the concatenation of the two transaction
        // hashes.
        block.vtx.push_back(MakeTransactionRef(CMutableTransaction{}));
        BOOST_CHECK(is_mutated(block, /*check_witness_root=*/false));
        HashWriter hasher;
        hasher.write(block.vtx[0]->GetHash());
        hasher.write(block.vtx[1]->GetHash());
        block.hashMerkleRoot = hasher.GetHash();
        BOOST_CHECK(is_not_mutated(block, /*check_witness_root=*/false));

        // Block with two transactions is mutated if any node is duplicate.
        {
            block.vtx[1] = block.vtx[0];
            HashWriter hasher;
            hasher.write(block.vtx[0]->GetHash());
            hasher.write(block.vtx[1]->GetHash());
            block.hashMerkleRoot = hasher.GetHash();
            BOOST_CHECK(is_mutated(block, /*check_witness_root=*/false));
        }

        // Blocks with 64-byte coinbase transactions are not considered mutated
        block.vtx.clear();
        {
            CMutableTransaction mtx;
            mtx.vin.resize(1);
            mtx.vout.resize(1);
            mtx.vout[0].scriptPubKey.resize(4);
            block.vtx.push_back(MakeTransactionRef(mtx));
            block.hashMerkleRoot = block.vtx.back()->GetHash();
            assert(block.vtx.back()->IsCoinBase());
            assert(GetSerializeSize(TX_NO_WITNESS(block.vtx.back())) == 64);
        }
        BOOST_CHECK(is_not_mutated(block, /*check_witness_root=*/false));
    }

    {
        // Test merkle root malleation

        // Pseudo code to mine transactions tx{1,2,3}:
        //
        // ```
        // loop {
        //   tx1 = random_tx()
        //   tx2 = random_tx()
        //   tx3 = deserialize_tx(txid(tx1) || txid(tx2));
        //   if serialized_size_without_witness(tx3) == 64 {
        //     print(hex(tx3))
        //     break
        //   }
        // }
        // ```
        //
        // The `random_tx` function used to mine the txs below simply created
        // empty transactions with a random version field.
        CMutableTransaction tx1;
        BOOST_CHECK(DecodeHexTx(tx1, "ff204bd0000000000000", /*try_no_witness=*/true, /*try_witness=*/false));
        CMutableTransaction tx2;
        BOOST_CHECK(DecodeHexTx(tx2, "8ae53c92000000000000", /*try_no_witness=*/true, /*try_witness=*/false));
        CMutableTransaction tx3;
        BOOST_CHECK(DecodeHexTx(tx3, "cdaf22d00002c6a7f848f8ae4d30054e61dcf3303d6fe01d282163341f06feecc10032b3160fcab87bdfe3ecfb769206ef2d991b92f8a268e423a6ef4d485f06", /*try_no_witness=*/true, /*try_witness=*/false));
        {
            // Verify that double_sha256(txid1||txid2) == txid3
            HashWriter hasher;
            hasher.write(tx1.GetHash());
            hasher.write(tx2.GetHash());
            assert(hasher.GetHash() == tx3.GetHash());
            // Verify that tx3 is 64 bytes in size (without witness).
            assert(GetSerializeSize(TX_NO_WITNESS(tx3)) == 64);
        }

        CBlock block;
        block.vtx.push_back(MakeTransactionRef(tx1));
        block.vtx.push_back(MakeTransactionRef(tx2));
        uint256 merkle_root = block.hashMerkleRoot = BlockMerkleRoot(block);
        BOOST_CHECK(is_not_mutated(block, /*check_witness_root=*/false));

        // Mutate the block by replacing the two transactions with one 64-byte
        // transaction that serializes into the concatenation of the txids of
        // the transactions in the unmutated block.
        block.vtx.clear();
        block.vtx.push_back(MakeTransactionRef(tx3));
        BOOST_CHECK(!block.vtx.back()->IsCoinBase());
        BOOST_CHECK(BlockMerkleRoot(block) == merkle_root);
        BOOST_CHECK(is_mutated(block, /*check_witness_root=*/false));
    }

    {
        CBlock block;
        block.vtx.push_back(create_coinbase_tx(/*include_witness=*/true));
        {
            CMutableTransaction mtx;
            mtx.vin.resize(1);
            mtx.vin[0].scriptWitness.stack.resize(1);
            mtx.vin[0].scriptWitness.stack[0] = {0};
            block.vtx.push_back(MakeTransactionRef(mtx));
        }
        block.hashMerkleRoot = BlockMerkleRoot(block);
        // Block with witnesses is considered mutated if the witness commitment
        // is not validated.
        BOOST_CHECK(is_mutated(block, /*check_witness_root=*/false));
        // Block with invalid witness commitment is considered mutated.
        BOOST_CHECK(is_mutated(block, /*check_witness_root=*/true));

        // Block with valid commitment is not mutated
        {
            auto commitment{BlockWitnessMerkleRoot(block)};
            insert_witness_commitment(block, commitment);
            block.hashMerkleRoot = BlockMerkleRoot(block);
        }
        BOOST_CHECK(is_not_mutated(block, /*check_witness_root=*/true));

        // Malleating witnesses should be caught by `IsBlockMutated`.
        {
            CMutableTransaction mtx{*block.vtx[1]};
            assert(!mtx.vin[0].scriptWitness.stack[0].empty());
            ++mtx.vin[0].scriptWitness.stack[0][0];
            block.vtx[1] = MakeTransactionRef(mtx);
        }
        // Without also updating the witness commitment, the merkle root should
        // not change when changing one of the witnesses.
        BOOST_CHECK(block.hashMerkleRoot == BlockMerkleRoot(block));
        BOOST_CHECK(is_mutated(block, /*check_witness_root=*/true));
        {
            auto commitment{BlockWitnessMerkleRoot(block)};
            insert_witness_commitment(block, commitment);
            block.hashMerkleRoot = BlockMerkleRoot(block);
        }
        BOOST_CHECK(is_not_mutated(block, /*check_witness_root=*/true));

        // Test malleating the coinbase witness reserved value
        {
            CMutableTransaction mtx{*block.vtx[0]};
            mtx.vin[0].scriptWitness.stack.resize(0);
            block.vtx[0] = MakeTransactionRef(mtx);
            block.hashMerkleRoot = BlockMerkleRoot(block);
        }
        BOOST_CHECK(is_mutated(block, /*check_witness_root=*/true));
    }
}

#ifdef ENABLE_BULLETPROOFS
static CBulletproof MakeBulletproof(CAmount value)
{
    static secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN);
    CBulletproof bp;
    unsigned char blind[32];
    GetRandBytes({blind, 32});

    BOOST_REQUIRE(secp256k1_pedersen_commit(ctx, &bp.commitment, blind, value, &secp256k1_generator_h) == 1);

    bp.proof.resize(SECP256K1_RANGE_PROOF_MAX_LENGTH);
    size_t proof_len = bp.proof.size();
    BOOST_REQUIRE(secp256k1_rangeproof_sign(ctx, bp.proof.data(), &proof_len, 0, &bp.commitment, blind,
                                           nullptr, 0, 0, value, &secp256k1_generator_h) == 1);
    bp.proof.resize(proof_len);
    bp.extra.assign(blind, blind + 32);
    return bp;
}

BOOST_AUTO_TEST_CASE(bulletproof_validation_tests)
{
    // Craft transaction with a Bulletproof opcode carrying an empty proof.
    CMutableTransaction mtx;
    mtx.version = CTransaction::CURRENT_VERSION | CTransaction::BULLETPROOF_VERSION;
    mtx.vin.emplace_back();
    mtx.vout.emplace_back(CAmount{0}, CScript());

    unsigned char commit[33]{};
    CScript bp_script;
    bp_script << OP_BULLETPROOF
              << std::vector<unsigned char>(commit, commit + 33)
              << std::vector<unsigned char>{};
    mtx.vout[0].scriptPubKey = bp_script;

    const CTransaction tx{mtx};
    TxValidationState state;
    BOOST_CHECK(!CheckBulletproofs(tx, state));
    BOOST_CHECK_EQUAL(state.GetRejectReason(), "bad-bulletproof");

    // Transaction without Bulletproof data should fail.
    CMutableTransaction mtx2;
    mtx2.version = CTransaction::CURRENT_VERSION | CTransaction::BULLETPROOF_VERSION;
    mtx2.vin.emplace_back();
    mtx2.vout.emplace_back(CAmount{0}, CScript());
    const CTransaction tx2{mtx2};
    TxValidationState state2;
    BOOST_CHECK(!CheckBulletproofs(tx2, state2));
    BOOST_CHECK_EQUAL(state2.GetRejectReason(), "bad-bulletproof-missing");

    // Output with non-zero value should be rejected.
    CMutableTransaction mtx3;
    mtx3.version = CTransaction::CURRENT_VERSION | CTransaction::BULLETPROOF_VERSION;
    mtx3.vin.emplace_back();
    mtx3.vout.emplace_back(CAmount{1}, CScript());
    CBulletproof bp3 = MakeBulletproof(/*value=*/1);
    CScript bp_script3;
    bp_script3 << OP_BULLETPROOF
               << std::vector<unsigned char>(bp3.commitment.data,
                                              bp3.commitment.data + sizeof(bp3.commitment.data))
               << bp3.proof;
    mtx3.vout[0].scriptPubKey = bp_script3;
    const CTransaction tx3{mtx3};
    TxValidationState state3;
    BOOST_CHECK(!CheckBulletproofs(tx3, state3));
    BOOST_CHECK_EQUAL(state3.GetRejectReason(), "bad-bulletproof-value");

    // Imbalanced input and output commitments should be rejected.
    CMutableTransaction mtx4;
    mtx4.version = CTransaction::CURRENT_VERSION | CTransaction::BULLETPROOF_VERSION;
    mtx4.vin.emplace_back();
    CBulletproof bp_in = MakeBulletproof(/*value=*/5);
    CScript in_script;
    in_script << OP_BULLETPROOF
              << std::vector<unsigned char>(bp_in.commitment.data,
                                             bp_in.commitment.data + sizeof(bp_in.commitment.data))
              << bp_in.proof;
    mtx4.vin[0].scriptSig = in_script;

    mtx4.vout.emplace_back(CAmount{0}, CScript());
    CBulletproof bp_out = MakeBulletproof(/*value=*/6);
    CScript out_script;
    out_script << OP_BULLETPROOF
               << std::vector<unsigned char>(bp_out.commitment.data,
                                              bp_out.commitment.data + sizeof(bp_out.commitment.data))
               << bp_out.proof;
    mtx4.vout[0].scriptPubKey = out_script;

    const CTransaction tx4{mtx4};
    TxValidationState state4;
    BOOST_CHECK(!CheckBulletproofs(tx4, state4));
    BOOST_CHECK_EQUAL(state4.GetRejectReason(), "bad-bulletproof-balance");
}

BOOST_FIXTURE_TEST_CASE(bulletproof_activation_tests, TestChain100Setup)
{
    // Create a standard mempool transaction and set the Bulletproof version bit.
    CMutableTransaction mtx = CreateValidMempoolTransaction(m_coinbase_txns[0], /*input_vout=*/0,
                                                           /*input_height=*/0, m_coinbase_key,
                                                           CScript() << OP_RETURN, /*output_amount=*/0,
                                                           /*submit=*/false);
    mtx.version |= CTransaction::BULLETPROOF_VERSION;
    const CTransactionRef tx = MakeTransactionRef(mtx);

    // Temporarily disable the deployment to simulate pre-activation.
    const auto& chainparams = Params();
    auto& dep = const_cast<Consensus::BIP9Deployment&>(chainparams.GetConsensus().vDeployments[Consensus::DEPLOYMENT_BULLETPROOF]);
    const auto old_start = dep.nStartTime;
    dep.nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE;

    MempoolAcceptResult res;
    {
        LOCK(cs_main);
        res = AcceptToMemoryPool(*m_node.chainman->ActiveChainstate(), tx, GetTime(), /*bypass_limits=*/false, /*test_accept=*/true);
    }
    BOOST_CHECK_EQUAL(res.m_state.GetRejectReason(), "bad-txns-bulletproof-premature");

    // Activate the deployment and verify that the same transaction is accepted.
    dep.nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
    {
        LOCK(cs_main);
        res = AcceptToMemoryPool(*m_node.chainman->ActiveChainstate(), tx, GetTime(), /*bypass_limits=*/false, /*test_accept=*/true);
    }
    BOOST_CHECK(res.m_result_type == MempoolAcceptResult::ResultType::VALID);

    // Restore original deployment parameters.
    dep.nStartTime = old_start;
}
#endif

BOOST_AUTO_TEST_CASE(pos_target_spacing)
{
    Consensus::Params params;
    params.nPowTargetSpacing = 10 * 60; // 10 minutes
    params.nPowTargetTimespan = 14 * 24 * 60 * 60;
    params.posLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
    params.nStakeTargetSpacing = 10 * 60;

    CBlockIndex indexLast;
    indexLast.nTime = 1000;
    arith_uint256 bn = UintToArith256(params.posLimit);
    bn >>= 1; // start below limit to avoid clamping
    indexLast.nBits = bn.GetCompact();

    const int64_t actual_spacing = params.nStakeTargetSpacing / 2;

    // Expected new target computed with non-default spacing
    arith_uint256 bnNew = bn;
    const int64_t interval = params.DifficultyAdjustmentInterval();
    const int64_t target_spacing = params.nStakeTargetSpacing;
    bnNew *= ((interval - 1) * target_spacing + 2 * actual_spacing);
    bnNew /= ((interval + 1) * target_spacing);
    const arith_uint256 bnLimit = UintToArith256(params.posLimit);
    if (bnNew <= 0 || bnNew > bnLimit) {
        bnNew = bnLimit;
    }
    const unsigned int expected = bnNew.GetCompact();

    const unsigned int result = GetPoSNextWorkRequired(&indexLast, indexLast.GetBlockTime() + actual_spacing, params);
    BOOST_CHECK_EQUAL(result, expected);
}

BOOST_AUTO_TEST_SUITE_END()
