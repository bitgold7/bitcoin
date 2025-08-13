#include <pos/stake.h>
#include <chain.h>
#include <chainparams.h>
#include <consensus/amount.h>
#include <coins.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <test/util/setup_common.h>
#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(stake_tests, BasicTestingSetup)

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

    BOOST_CHECK(CheckStakeKernelHash(&prev_index, nBits, hash_block_from, nTimeBlockFrom,
                                     amount, prevout, nTimeTx, hash_proof, false));
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

    BOOST_CHECK(!CheckStakeKernelHash(&prev_index, nBits, hash_block_from, nTimeBlockFrom,
                                      amount, prevout, nTimeTx, hash_proof, false));
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

    BOOST_CHECK(!CheckStakeKernelHash(&prev_index, nBits, hash_block_from, nTimeBlockFrom,
                                      amount, prevout, nTimeTx, hash_proof, false));
}

BOOST_AUTO_TEST_CASE(coinstake_requires_two_outputs)
{
    // Setup a simple chain with a genesis and previous block
    uint256 prev_hash{1};
    uint256 genesis_hash{2};

    CBlockIndex genesis;
    genesis.nHeight = 0;
    genesis.nTime = 0;
    genesis.phashBlock = &genesis_hash;

    CBlockIndex prev_index;
    prev_index.pprev = &genesis;
    prev_index.nHeight = 1;
    prev_index.nTime = 100;
    prev_index.phashBlock = &prev_hash;

    CChain chain;
    chain.SetTip(&prev_index);

    // Prepare staking UTXO
    CCoinsView view_dummy;
    CCoinsViewCache view(&view_dummy);
    COutPoint prevout{Txid::FromUint256(uint256{3}), 0};
    Coin coin{CTxOut{100 * COIN, CScript{}}, /*nHeight=*/0, /*fCoinBase=*/false};
    view.AddCoin(prevout, std::move(coin), false);

    // Build block with invalid coinstake (missing second output)
    CMutableTransaction coinbase;
    coinbase.vin.emplace_back();
    coinbase.vout.emplace_back();

    CMutableTransaction coinstake;
    coinstake.vin.emplace_back(prevout);
    coinstake.vout.resize(1);
    coinstake.vout[0].SetNull();

    CBlock block;
    block.nTime = MIN_STAKE_AGE;
    block.nBits = 0x207fffff;
    block.vtx = {MakeTransactionRef(coinbase), MakeTransactionRef(coinstake)};

    BOOST_CHECK(!ContextualCheckProofOfStake(block, &prev_index, view, chain, Params().GetConsensus()));

    // Add a second output and verify validation succeeds
    coinstake.vout.resize(2);
    coinstake.vout[0].SetNull();
    coinstake.vout[1].nValue = 100 * COIN;
    block.vtx[1] = MakeTransactionRef(coinstake);

    BOOST_CHECK(ContextualCheckProofOfStake(block, &prev_index, view, chain, Params().GetConsensus()));
}

BOOST_AUTO_TEST_SUITE_END()
