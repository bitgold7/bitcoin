#include <arith_uint256.h>
#include <boost/test/unit_test.hpp>
#include <chain.h>
#include <chainparams.h>
#include <consensus/amount.h>
#include <consensus/params.h>
#include <consensus/pos/stake.h>
#include <consensus/pos/difficulty.h>
#include <primitives/transaction.h>
#include <primitives/block.h>
#include <random>
#include <test/util/setup_common.h>
#include <vector>

BOOST_FIXTURE_TEST_SUITE(pos_difficulty_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(target_spacing_convergence)
{
    const Consensus::Params& params = Params().GetConsensus();
    const arith_uint256 bnLimit = UintToArith256(params.posLimit);

    std::vector<std::unique_ptr<CBlockIndex>> chain;
    chain.emplace_back(std::make_unique<CBlockIndex>());
    CBlockIndex* prev = chain.back().get();
    prev->nHeight = 0;
    prev->nBits = bnLimit.GetCompact();
    prev->nTime = 0;

    std::mt19937 rng{0xdeadbeef};
    std::uniform_int_distribution<int> jitter_dist{-4, 4};

    int64_t total_spacing = 0;
    const int iterations = 200;
    for (int i = 0; i < iterations; ++i) {
        arith_uint256 bnPrev;
        bnPrev.SetCompact(prev->nBits);
        double ratio = bnLimit.getdouble() / bnPrev.getdouble();
        int64_t base_spacing = static_cast<int64_t>(params.nStakeTargetSpacing * ratio);
        int64_t jitter = jitter_dist(rng) * 16;
        int64_t spacing = base_spacing + jitter;
        if (spacing < 16) spacing = 16;
        spacing &= ~static_cast<int64_t>(params.nStakeTimestampMask);
        int64_t block_time = prev->nTime + spacing;
        total_spacing += spacing;

        unsigned int nBits = GetPoSNextTargetRequired(prev, block_time, params);
        auto next = std::make_unique<CBlockIndex>();
        next->pprev = prev;
        next->nHeight = prev->nHeight + 1;
        next->nTime = block_time;
        next->nBits = nBits;
        chain.emplace_back(std::move(next));
        prev = chain.back().get();
    }

    double avg_spacing = static_cast<double>(total_spacing) / iterations;
    BOOST_CHECK_CLOSE(avg_spacing, static_cast<double>(params.nStakeTargetSpacing), 1.0);
}

BOOST_AUTO_TEST_CASE(stake_timestamp_mask)
{
    const Consensus::Params& params = Params().GetConsensus();
    CBlockHeader header;

    unsigned int prev_time = (GetTime() & ~params.nStakeTimestampMask) - params.nStakeTargetSpacing;

    // Valid timestamp: divisible by mask, after previous block, and not far in the future
    header.nTime = prev_time + params.nStakeTargetSpacing;
    BOOST_CHECK(kernel::CheckStakeTimestamp(header, prev_time, params) == kernel::StakeTimeValidationResult::OK);

    // Fails mask divisibility
    header.nTime |= 1;
    BOOST_CHECK(kernel::CheckStakeTimestamp(header, prev_time, params) == kernel::StakeTimeValidationResult::MASK);

    // Valid mask but too far in the future
    header.nTime = (GetTime() + 16) & ~params.nStakeTimestampMask;
    BOOST_CHECK(kernel::CheckStakeTimestamp(header, prev_time, params) == kernel::StakeTimeValidationResult::FUTURE);

    // Too early relative to previous block
    header.nTime = prev_time;
    BOOST_CHECK(kernel::CheckStakeTimestamp(header, prev_time, params) == kernel::StakeTimeValidationResult::SPACING);
}

BOOST_AUTO_TEST_CASE(kernel_hash_interval_rejection)
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
    unsigned int good_time = MIN_STAKE_AGE;
    Consensus::Params params;
    params.fEnforceStakeTimeInterval = true;

    node::StakeModifierManager man;
    man.UpdateOnConnect(&prev_index, params);

    BOOST_CHECK(CheckStakeKernelHash(&prev_index, nBits, hash_block_from, nTimeBlockFrom,
                                     amount, prevout, good_time, man, hash_proof, false, params));

    unsigned int bad_time = good_time + (params.nStakeTimestampMask + 1);
    BOOST_CHECK(!CheckStakeKernelHash(&prev_index, nBits, hash_block_from, nTimeBlockFrom,
                                      amount, prevout, bad_time, man, hash_proof, false, params));
}

BOOST_AUTO_TEST_SUITE_END()
