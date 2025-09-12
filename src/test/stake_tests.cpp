#include <pos/stake.h>
#include <pos/stakemodifier.h>
#include <node/stake_modifier_manager.h>
#include <pos/difficulty.h>
#include <chain.h>
#include <kernel/chain.h>
#include <consensus/amount.h>
#include <consensus/merkle.h>
#include <chainparams.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <hash.h>
#include <script/script.h>
#include <validation.h>
#include <test/util/setup_common.h>
#include <boost/test/unit_test.hpp>
#include <memory>

BOOST_FIXTURE_TEST_SUITE(stake_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(target_weight_multiplication)
{
    arith_uint256 target{1};
    BOOST_CHECK(MultiplyStakeTarget(target, 2) == arith_uint256{2});

    arith_uint256 max_target = ~arith_uint256();
    BOOST_CHECK(MultiplyStakeTarget(max_target, 2) == ~arith_uint256());
}

BOOST_AUTO_TEST_CASE(valid_kernel)
{
    // Construct a dummy previous block index
    uint256 prev_hash{1};
    CBlockIndex prev_index;
    prev_index.nHeight = 1;
    prev_index.nTime = 100;
    prev_index.phashBlock = &prev_hash;

    uint256 hash_block_from{2};
    unsigned int nTimeBlockFrom = 0;
    CAmount amount = 100 * COIN;
    COutPoint prevout{Txid::FromUint256(uint256{3}), 0};

    uint256 hash_proof;
    unsigned int nBits = 0x207fffff; // very low difficulty
    unsigned int nTimeTx = nTimeBlockFrom + MIN_STAKE_AGE; // exactly minimum age
    Consensus::Params params;

    node::StakeModifierManager man;
    man.UpdateOnConnect(&prev_index, params);

    BOOST_CHECK(CheckStakeKernelHash(&prev_index, nBits, hash_block_from, nTimeBlockFrom,
                                     amount, prevout, nTimeTx, man, hash_proof, false, params));
}

BOOST_AUTO_TEST_CASE(invalid_kernel_time)
{
    uint256 prev_hash{1};
    CBlockIndex prev_index;
    prev_index.nHeight = 1;
    prev_index.nTime = 100;
    prev_index.phashBlock = &prev_hash;

    uint256 hash_block_from{2};
    unsigned int nTimeBlockFrom = 0;
    CAmount amount = 100 * COIN;
    COutPoint prevout{Txid::FromUint256(uint256{3}), 0};

    uint256 hash_proof;
    unsigned int nBits = 0x207fffff;
    unsigned int nTimeTx = MIN_STAKE_AGE - 16; // not old enough and masked
    Consensus::Params params;

    node::StakeModifierManager man;
    man.UpdateOnConnect(&prev_index, params);

    BOOST_CHECK(!CheckStakeKernelHash(&prev_index, nBits, hash_block_from, nTimeBlockFrom,
                                      amount, prevout, nTimeTx, man, hash_proof, false, params));
}

BOOST_AUTO_TEST_CASE(invalid_kernel_target)
{
    uint256 prev_hash{1};
    CBlockIndex prev_index;
    prev_index.nHeight = 1;
    prev_index.nTime = 100;
    prev_index.phashBlock = &prev_hash;

    uint256 hash_block_from{2};
    unsigned int nTimeBlockFrom = 0;
    CAmount amount = 1 * COIN; // minimum stake amount
    COutPoint prevout{Txid::FromUint256(uint256{3}), 0};

    uint256 hash_proof;
    unsigned int nBits = 0x1;     // extremely high difficulty
    unsigned int nTimeTx = MIN_STAKE_AGE;    // minimal age
    Consensus::Params params;

    node::StakeModifierManager man;
    man.UpdateOnConnect(&prev_index, params);

    BOOST_CHECK(!CheckStakeKernelHash(&prev_index, nBits, hash_block_from, nTimeBlockFrom,
                                      amount, prevout, nTimeTx, man, hash_proof, false, params));
}

BOOST_AUTO_TEST_CASE(invalid_kernel_amount)
{
    uint256 prev_hash{1};
    CBlockIndex prev_index;
    prev_index.nHeight = 1;
    prev_index.nTime = 100;
    prev_index.phashBlock = &prev_hash;

    uint256 hash_block_from{2};
    unsigned int nTimeBlockFrom = 0;
    CAmount amount = 0; // zero stake amount
    COutPoint prevout{Txid::FromUint256(uint256{3}), 0};

    uint256 hash_proof;
    unsigned int nBits = 0x207fffff;
    unsigned int nTimeTx = MIN_STAKE_AGE;
    Consensus::Params params;

    StakeModifierManager& man = GetStakeModifierManager();
    man = StakeModifierManager();
    man.UpdateOnConnect(&prev_index, params);

    BOOST_CHECK(!CheckStakeKernelHash(&prev_index, nBits, hash_block_from, nTimeBlockFrom,
                                      amount, prevout, nTimeTx, hash_proof, false, params));
}

BOOST_AUTO_TEST_CASE(kernel_hash_matches_expectation)
{
    uint256 prev_hash{1};
    CBlockIndex prev_index;
    prev_index.nHeight = 1;
    prev_index.nTime = 100;
    prev_index.phashBlock = &prev_hash;

    uint256 hash_block_from{2};
    unsigned int nTimeBlockFrom = 0;
    CAmount amount = 100 * COIN;
    COutPoint prevout{Txid::FromUint256(uint256{3}), 0};

    unsigned int nBits = 0x207fffff;
    unsigned int nTimeTx = MIN_STAKE_AGE;

    Consensus::Params params;
    node::StakeModifierManager man;
    man.UpdateOnConnect(&prev_index, params);
    uint256 stake_mod = man.GetCurrentModifier();

    uint256 expected_hash;
    {
        HashWriter ss_kernel;
        ss_kernel << stake_mod << prevout.hash << prevout.n << nTimeBlockFrom << nTimeTx;
        expected_hash = ss_kernel.GetHash();
    }

    uint256 hash_proof;
    BOOST_CHECK(CheckStakeKernelHash(&prev_index, nBits, hash_block_from, nTimeBlockFrom,
                                     amount, prevout, nTimeTx, man, hash_proof, false, params));
    BOOST_CHECK_EQUAL(hash_proof, expected_hash);
}

BOOST_AUTO_TEST_CASE(stake_modifier_differs_per_input)
{
    uint256 prev_hash{1};
    CBlockIndex prev_index;
    prev_index.nHeight = 1;
    prev_index.nTime = 100;
    prev_index.phashBlock = &prev_hash;

    uint256 hash_block_from{2};
    unsigned int nTimeBlockFrom = 0;
    unsigned int nBits = 0x207fffff;
    unsigned int nTimeTx = MIN_STAKE_AGE;

    COutPoint prevout1{Txid::FromUint256(uint256{3}), 0};
    COutPoint prevout2{Txid::FromUint256(uint256{4}), 1};

    Consensus::Params params;
    node::StakeModifierManager man;
    man.UpdateOnConnect(&prev_index, params);

    uint256 proof1;
    BOOST_CHECK(CheckStakeKernelHash(&prev_index, nBits, hash_block_from, nTimeBlockFrom,
                                     100 * COIN, prevout1, nTimeTx, man, proof1, false, params));
    uint256 proof2;
    BOOST_CHECK(CheckStakeKernelHash(&prev_index, nBits, hash_block_from, nTimeBlockFrom,
                                     100 * COIN, prevout2, nTimeTx, man, proof2, false, params));
    BOOST_CHECK(proof1 != proof2);
}

BOOST_AUTO_TEST_CASE(stake_modifier_refresh)
{
    uint256 prev_hash{1};
    CBlockIndex prev_index;
    prev_index.nHeight = 10;
    prev_index.nTime = 1000;
    prev_index.phashBlock = &prev_hash;

    Consensus::Params params;
    params.nStakeModifierInterval = 60; // short interval for test

    node::StakeModifierManager man;

    // Connect initial block and compute modifier
    man.UpdateOnConnect(&prev_index, params);
    uint256 mod1 = man.GetCurrentModifier();
    uint256 exp1 = ComputeStakeModifier(prev_index.pprev, uint256{});
    BOOST_CHECK_EQUAL(mod1, exp1);

    // Within interval, connecting next block should not change modifier
    CBlockIndex next_index;
    uint256 next_hash{2};
    next_index.nHeight = 11;
    next_index.nTime = 1016;
    next_index.pprev = &prev_index;
    next_index.phashBlock = &next_hash;
    man.UpdateOnConnect(&next_index, params);
    uint256 mod2 = man.GetCurrentModifier();
    BOOST_CHECK_EQUAL(mod1, mod2);

    // After interval passes, connecting another block updates modifier
    CBlockIndex next_index2;
    uint256 next_hash2{3};
    next_index2.nHeight = 12;
    next_index2.nTime = 1061;
    next_index2.pprev = &next_index;
    next_index2.phashBlock = &next_hash2;
    man.UpdateOnConnect(&next_index2, params);
    uint256 mod3 = man.GetCurrentModifier();
    uint256 exp3 = ComputeStakeModifier(&next_index2, mod1);
    BOOST_CHECK_EQUAL(mod3, exp3);
    BOOST_CHECK(mod3 != mod1);
}

BOOST_AUTO_TEST_CASE(stake_modifier_dynamic_refresh)
{
    uint256 prev_hash{1};
    CBlockIndex prev_index;
    prev_index.nHeight = 10;
    prev_index.nTime = 1000;
    prev_index.phashBlock = &prev_hash;

    Consensus::Params params;
    params.nStakeModifierInterval = 60; // short interval for test
    params.nStakeModifierVersion = 3;

    node::StakeModifierManager man;
    man.UpdateOnConnect(&prev_index, params);
    uint256 mod1 = man.GetCurrentModifier();

    // Within interval, modifier remains the same
    uint256 mod2 = man.GetDynamicModifier(&prev_index, 1030, params);
    BOOST_CHECK_EQUAL(mod1, mod2);

    // After interval passes, modifier refreshes
    uint256 mod3 = man.GetDynamicModifier(&prev_index, 1101, params);
    uint256 exp3 = ComputeStakeModifier(&prev_index, mod1);
    BOOST_CHECK_EQUAL(mod3, exp3);
    BOOST_CHECK(mod3 != mod1);
}

BOOST_AUTO_TEST_CASE(stake_modifier_reorg)
{
    // Verify modifier rolls back correctly on reorg
    uint256 h1{1}, h2{2}, h3{3};
    CBlockIndex b1; b1.nHeight = 1; b1.nTime = 1000; b1.phashBlock = &h1;
    CBlockIndex b2; b2.nHeight = 2; b2.nTime = 1020; b2.pprev = &b1; b2.phashBlock = &h2;
    CBlockIndex b3; b3.nHeight = 3; b3.nTime = 1100; b3.pprev = &b2; b3.phashBlock = &h3;

    Consensus::Params params; params.nStakeModifierInterval = 60;
    node::StakeModifierManager man;

    man.UpdateOnConnect(&b1, params);
    uint256 mod1 = man.GetCurrentModifier();
    man.UpdateOnConnect(&b2, params);
    uint256 mod2 = man.GetCurrentModifier();
    BOOST_CHECK_EQUAL(mod1, mod2); // interval not passed
    man.UpdateOnConnect(&b3, params);
    uint256 mod3 = man.GetCurrentModifier();
    BOOST_CHECK(mod3 != mod2);

    man.RemoveOnDisconnect(&b3);
    BOOST_CHECK_EQUAL(man.GetCurrentModifier(), mod2);
    man.RemoveOnDisconnect(&b2);
    BOOST_CHECK_EQUAL(man.GetCurrentModifier(), mod1);
}

BOOST_AUTO_TEST_CASE(stake_modifier_tip_after_reorg)
{
    const Consensus::Params& params = Params().GetConsensus();
    node::StakeModifierManager man;

    uint256 h1{1}, h2{2}, h3{3}, h2a{4}, h3a{5};
    CBlockIndex b1; b1.nHeight = 1; b1.nTime = 1000; b1.phashBlock = &h1;
    CBlockIndex b2; b2.nHeight = 2; b2.nTime = 1000 + params.nStakeModifierInterval / 2; b2.pprev = &b1; b2.phashBlock = &h2;
    CBlockIndex b3; b3.nHeight = 3; b3.nTime = 1000 + params.nStakeModifierInterval + 10; b3.pprev = &b2; b3.phashBlock = &h3;

    // Alternate branch
    CBlockIndex b2a; b2a.nHeight = 2; b2a.nTime = 1000 + params.nStakeModifierInterval / 3; b2a.pprev = &b1; b2a.phashBlock = &h2a;
    CBlockIndex b3a; b3a.nHeight = 3; b3a.nTime = 1000 + params.nStakeModifierInterval * 2; b3a.pprev = &b2a; b3a.phashBlock = &h3a;

    // Connect initial chain
    man.BlockConnected(ChainstateRole::NORMAL, nullptr, &b1);
    uint256 mod1 = man.GetCurrentModifier();
    man.BlockConnected(ChainstateRole::NORMAL, nullptr, &b2);
    man.BlockConnected(ChainstateRole::NORMAL, nullptr, &b3);

    // Reorg to alternative branch
    man.BlockDisconnected(nullptr, &b3);
    man.BlockDisconnected(nullptr, &b2);
    man.BlockConnected(ChainstateRole::NORMAL, nullptr, &b2a);
    man.BlockConnected(ChainstateRole::NORMAL, nullptr, &b3a);

    uint256 expected = ComputeStakeModifier(&b2a, mod1);
    BOOST_CHECK_EQUAL(man.GetCurrentModifier(), expected);
}

BOOST_AUTO_TEST_CASE(coinstake_structure)
{
    // Valid coinstake transaction in second position
    CMutableTransaction coinbase;
    coinbase.vin.resize(1);
    coinbase.vin[0].prevout.SetNull();
    coinbase.vout.resize(1);

    CMutableTransaction coinstake;
    coinstake.vin.resize(1);
    coinstake.vout.resize(2);
    coinstake.vout[0].SetNull();

    CBlock block;
    block.vtx.emplace_back(MakeTransactionRef(coinbase));
    block.vtx.emplace_back(MakeTransactionRef(coinstake));
    BOOST_CHECK(IsProofOfStake(block));

    // Missing coinstake
    CBlock block_no;
    block_no.vtx.emplace_back(MakeTransactionRef(coinbase));
    BOOST_CHECK(!IsProofOfStake(block_no));

    // Bad coinstake first output not empty
    CMutableTransaction bad;
    bad.vin.resize(1);
    bad.vout.resize(2);
    bad.vout[0].nValue = 1; // non-null output breaks coinstake rules
    CBlock block_bad;
    block_bad.vtx.emplace_back(MakeTransactionRef(coinbase));
    block_bad.vtx.emplace_back(MakeTransactionRef(bad));
    BOOST_CHECK(!IsProofOfStake(block_bad));
}

BOOST_AUTO_TEST_CASE(height1_requires_coinstake)
{
    uint256 prev_hash{1};
    CBlockIndex prev_index;
    prev_index.nHeight = 0;
    prev_index.nTime = 0;
    prev_index.phashBlock = &prev_hash;

    CChain chain;
    chain.SetTip(prev_index);

    CCoinsView view_base;
    CCoinsViewCache view(&view_base);
    Consensus::Params params;

    CBlock block;
    block.nTime = 16;

    CMutableTransaction coinbase;
    coinbase.vin.resize(1);
    coinbase.vin[0].prevout.SetNull();
    coinbase.vout.resize(1);
    block.vtx.emplace_back(MakeTransactionRef(std::move(coinbase)));

    BOOST_CHECK(!ContextualCheckProofOfStake(block, &prev_index, view, chain, params));
}

BOOST_AUTO_TEST_CASE(valid_height1_coinstake)
{
    uint256 prev_hash{1};
    CBlockIndex prev_index;
    prev_index.nHeight = 0;
    prev_index.nTime = 0;
    prev_index.nBits = 0x207fffff;
    prev_index.phashBlock = &prev_hash;

    CChain chain;
    chain.SetTip(prev_index);

    CCoinsView view_base;
    CCoinsViewCache view(&view_base);

    // Add a mature coin to the view
    COutPoint prevout{Txid::FromUint256(uint256{2}), 0};
    Coin coin;
    coin.out.nValue = 1 * COIN;
    coin.nHeight = 0;
    view.AddCoin(prevout, std::move(coin), false);

    Consensus::Params params;
    CBlock block;
    block.nTime = params.nStakeMinAge + 16;
    block.nBits = GetPoSNextTargetRequired(&prev_index, block.nTime, params);

    CMutableTransaction coinbase;
    coinbase.vin.resize(1);
    coinbase.vin[0].prevout.SetNull();
    coinbase.vout.resize(1);
    block.vtx.emplace_back(MakeTransactionRef(coinbase));

    CMutableTransaction coinstake;
    coinstake.nLockTime = block.nTime;
    coinstake.vin.emplace_back(prevout);
    coinstake.vin[0].nSequence = CTxIn::SEQUENCE_FINAL;
    coinstake.vout.resize(2);
    coinstake.vout[0].SetNull();
    coinstake.vout[1].nValue = 1 * COIN;
    block.vtx.emplace_back(MakeTransactionRef(std::move(coinstake)));

    BOOST_CHECK(ContextualCheckProofOfStake(block, &prev_index, view, chain, params));
}

BOOST_AUTO_TEST_CASE(reject_low_stake_amount)
{
    uint256 prev_hash{1};
    CBlockIndex prev_index;
    prev_index.nHeight = 0;
    prev_index.nTime = 0;
    prev_index.nBits = 0x207fffff;
    prev_index.phashBlock = &prev_hash;

    CChain chain;
    chain.SetTip(prev_index);

    CCoinsView view_base;
    CCoinsViewCache view(&view_base);

    COutPoint prevout{Txid::FromUint256(uint256{2}), 0};
    Coin coin;
    coin.out.nValue = COIN / 2; // below minimum stake amount
    coin.nHeight = 0;
    view.AddCoin(prevout, std::move(coin), false);

    Consensus::Params params;
    CBlock block;
    block.nTime = params.nStakeMinAge + 16;
    block.nBits = GetPoSNextTargetRequired(&prev_index, block.nTime, params);

    CMutableTransaction coinbase;
    coinbase.vin.resize(1);
    coinbase.vin[0].prevout.SetNull();
    coinbase.vout.resize(1);
    block.vtx.emplace_back(MakeTransactionRef(coinbase));

    CMutableTransaction coinstake;
    coinstake.nLockTime = block.nTime;
    coinstake.vin.emplace_back(prevout);
    coinstake.vin[0].nSequence = CTxIn::SEQUENCE_FINAL;
    coinstake.vout.resize(2);
    coinstake.vout[0].SetNull();
    coinstake.vout[1].nValue = COIN / 2;
    block.vtx.emplace_back(MakeTransactionRef(std::move(coinstake)));

    BOOST_CHECK(!ContextualCheckProofOfStake(block, &prev_index, view, chain, params));
}

BOOST_AUTO_TEST_CASE(height1_allows_young_coinstake)
{
    uint256 prev_hash{1};
    CBlockIndex prev_index;
    prev_index.nHeight = 0;
    prev_index.nTime = 16; // young genesis time
    prev_index.nBits = 0x207fffff;
    prev_index.phashBlock = &prev_hash;

    CChain chain;
    chain.SetTip(prev_index);

    CCoinsView view_base;
    CCoinsViewCache view(&view_base);

    // Add a coin from the genesis block
    COutPoint prevout{Txid::FromUint256(uint256{2}), 0};
    Coin coin;
    coin.out.nValue = 1 * COIN;
    coin.nHeight = 0;
    view.AddCoin(prevout, std::move(coin), false);

    Consensus::Params params;
    CBlock block;
    block.nTime = params.nStakeMinAge; // younger than required age relative to prev_index
    block.nBits = GetPoSNextTargetRequired(&prev_index, block.nTime, params);

    CMutableTransaction coinbase;
    coinbase.vin.resize(1);
    coinbase.vin[0].prevout.SetNull();
    coinbase.vout.resize(1);
    block.vtx.emplace_back(MakeTransactionRef(coinbase));

    CMutableTransaction coinstake;
    coinstake.nLockTime = block.nTime;
    coinstake.vin.emplace_back(prevout);
    coinstake.vin[0].nSequence = CTxIn::SEQUENCE_FINAL;
    coinstake.vout.resize(2);
    coinstake.vout[0].SetNull();
    coinstake.vout[1].nValue = 1 * COIN;
    block.vtx.emplace_back(MakeTransactionRef(std::move(coinstake)));

    BOOST_CHECK(ContextualCheckProofOfStake(block, &prev_index, view, chain, params));
}

BOOST_FIXTURE_TEST_CASE(reject_pow_after_height1, ChainTestingSetup)
{
    g_chainman = m_node.chainman.get();

    uint256 prev_hash{1};
    {
        LOCK(cs_main);
        auto& map = g_chainman->BlockIndex();
        auto [it, inserted] = map.emplace(std::piecewise_construct,
                                         std::forward_as_tuple(prev_hash),
                                         std::forward_as_tuple());
        it->second.nHeight = 1;
        it->second.nBits = 0x207fffff;
        it->second.phashBlock = &prev_hash;
    }

    CBlock block;
    block.hashPrevBlock = prev_hash;
    block.nBits = 0x207fffff;
    block.nTime = 2;

    CMutableTransaction coinbase;
    coinbase.vin.resize(1);
    coinbase.vin[0].prevout.SetNull();
    coinbase.vout.resize(1);
    block.vtx.emplace_back(MakeTransactionRef(std::move(coinbase)));
    block.hashMerkleRoot = BlockMerkleRoot(block);

    BlockValidationState state;
    BOOST_CHECK(!CheckBlock(block, state, Params().GetConsensus()));
    BOOST_CHECK_EQUAL(state.GetRejectReason(), "bad-pow");

    {
        LOCK(cs_main);
        g_chainman->BlockIndex().erase(prev_hash);
    }
    g_chainman = nullptr;
}

BOOST_FIXTURE_TEST_CASE(process_new_block_rejects_pow_height2, ChainTestingSetup)
{
    g_chainman = m_node.chainman.get();

    uint256 prev_hash{1};
    {
        LOCK(cs_main);
        auto& map = g_chainman->BlockIndex();
        auto [it, inserted] = map.emplace(std::piecewise_construct,
                                         std::forward_as_tuple(prev_hash),
                                         std::forward_as_tuple());
        it->second.nHeight = 1;
        it->second.nBits = 0x207fffff;
        it->second.phashBlock = &prev_hash;
    }

    CBlock block;
    block.hashPrevBlock = prev_hash;
    block.nBits = 0x207fffff;
    block.nTime = 2;

    CMutableTransaction coinbase;
    coinbase.vin.resize(1);
    coinbase.vin[0].prevout.SetNull();
    coinbase.vout.resize(1);
    block.vtx.emplace_back(MakeTransactionRef(std::move(coinbase)));
    block.hashMerkleRoot = BlockMerkleRoot(block);

    bool new_block{false};
    BOOST_CHECK(!g_chainman->ProcessNewBlock(std::make_shared<const CBlock>(block),
                                             /*force_processing=*/true, /*min_pow_checked=*/true, &new_block));

    {
        LOCK(cs_main);
        g_chainman->BlockIndex().erase(prev_hash);
    }
    g_chainman = nullptr;
}

BOOST_AUTO_TEST_CASE(stake_modifier_version_selection)
{
    // Set up a previous block index
    uint256 prev_hash{1};
    CBlockIndex prev_index;
    prev_index.nHeight = 1;
    prev_index.nTime = 100;
    prev_index.phashBlock = &prev_hash;

    uint256 hash_block_from{2};
    unsigned int nTimeBlockFrom = 0;
    CAmount amount = 100 * COIN;
    COutPoint prevout{Txid::FromUint256(uint256{3}), 0};
    unsigned int nBits = 0x207fffff;
    unsigned int nTimeTx = MIN_STAKE_AGE;

    // Version 0 uses the legacy modifier routine
    Consensus::Params params0;
    params0.nStakeModifierVersion = 0;
    uint256 mod0 = GetStakeModifier(&prev_index, nTimeTx, params0);
    HashWriter ss0;
    ss0 << mod0 << prevout.hash << prevout.n << nTimeBlockFrom << nTimeTx;
    uint256 expect0 = ss0.GetHash();
    uint256 proof0;
    node::StakeModifierManager dummy_man; // unused for version 0
    BOOST_CHECK(CheckStakeKernelHash(&prev_index, nBits, hash_block_from, nTimeBlockFrom,
                                     amount, prevout, nTimeTx, dummy_man, proof0, false, params0));
    BOOST_CHECK_EQUAL(proof0, expect0);

    // Version 1 pulls the modifier from the manager
    Consensus::Params params1;
    params1.nStakeModifierVersion = 1;
    node::StakeModifierManager man1;
    man1.UpdateOnConnect(&prev_index, params1);
    uint256 mod1 = man1.GetCurrentModifier();
    HashWriter ss1;
    ss1 << mod1 << prevout.hash << prevout.n << nTimeBlockFrom << nTimeTx;
    uint256 expect1 = ss1.GetHash();
    uint256 proof1;
    BOOST_CHECK(CheckStakeKernelHash(&prev_index, nBits, hash_block_from, nTimeBlockFrom,
                                     amount, prevout, nTimeTx, man1, proof1, false, params1));
    BOOST_CHECK_EQUAL(proof1, expect1);

    // Version 3 refreshes the modifier based on time
    Consensus::Params params3;
    params3.nStakeModifierVersion = 3;
    node::StakeModifierManager man3;
    man3.UpdateOnConnect(&prev_index, params3);
    uint256 mod3 = man3.GetDynamicModifier(&prev_index, nTimeTx, params3);
    HashWriter ss3;
    ss3 << mod3 << prevout.hash << prevout.n << nTimeBlockFrom << nTimeTx;
    uint256 expect3 = ss3.GetHash();
    uint256 proof3;
    BOOST_CHECK(CheckStakeKernelHash(&prev_index, nBits, hash_block_from, nTimeBlockFrom,
                                     amount, prevout, nTimeTx, man3, proof3, false, params3));
    BOOST_CHECK_EQUAL(proof3, expect3);
}

BOOST_AUTO_TEST_SUITE_END()
