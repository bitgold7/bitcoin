// Copyright (c) 2017-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

#include <chain.h>
#include <node/blockstorage.h>
#include <rpc/blockchain.h>
#include <sync.h>
#include <test/util/setup_common.h>
#include <util/string.h>

#include <cstdlib>

using util::ToString;

/* Equality between doubles is imprecise. Comparison should be done
 * with a small threshold of tolerance, rather than exact equality.
 */
BOOST_FIXTURE_TEST_SUITE(blockchain_tests, BasicTestingSetup)

//! Prune chain from height down to genesis block and check that
//! GetPruneHeight returns the correct value
static void CheckGetPruneHeight(node::BlockManager& blockman, CChain& chain, int height) EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
{
    AssertLockHeld(::cs_main);

    // Emulate pruning all blocks from `height` down to the genesis block
    // by unsetting the `BLOCK_HAVE_DATA` flag from `nStatus`
    for (CBlockIndex* it{chain[height]}; it != nullptr && it->nHeight > 0; it = it->pprev) {
        it->nStatus &= ~BLOCK_HAVE_DATA;
    }

    const auto prune_height{GetPruneHeight(blockman, chain)};
    BOOST_REQUIRE(prune_height.has_value());
    BOOST_CHECK_EQUAL(*prune_height, height);
}

BOOST_FIXTURE_TEST_CASE(get_prune_height, TestChain100Setup)
{
    LOCK(::cs_main);
    auto& chain = m_node.chainman->ActiveChain();
    auto& blockman = m_node.chainman->m_blockman;

    // Fresh chain of 100 blocks without any pruned blocks, so std::nullopt should be returned
    BOOST_CHECK(!GetPruneHeight(blockman, chain).has_value());

    // Start pruning
    CheckGetPruneHeight(blockman, chain, 1);
    CheckGetPruneHeight(blockman, chain, 99);
    CheckGetPruneHeight(blockman, chain, 100);
}

BOOST_AUTO_TEST_CASE(num_chain_tx_max)
{
    CBlockIndex block_index{};
    block_index.m_chain_tx_count = std::numeric_limits<uint64_t>::max();
    BOOST_CHECK_EQUAL(block_index.m_chain_tx_count, std::numeric_limits<uint64_t>::max());
}

BOOST_FIXTURE_TEST_CASE(invalidate_block, TestChain100Setup)
{
    const CChain& active{*WITH_LOCK(Assert(m_node.chainman)->GetMutex(), return &Assert(m_node.chainman)->ActiveChain())};

    // Check BlockStatus when doing InvalidateBlock()
    BlockValidationState state;
    auto* orig_tip = active.Tip();
    int height_to_invalidate = orig_tip->nHeight - 10;
    auto* tip_to_invalidate = active[height_to_invalidate];
    m_node.chainman->ActiveChainstate().InvalidateBlock(state, tip_to_invalidate);

    // tip_to_invalidate just got invalidated, so it's BLOCK_FAILED_VALID
    WITH_LOCK(::cs_main, assert(tip_to_invalidate->nStatus & BLOCK_FAILED_VALID));
    WITH_LOCK(::cs_main, assert((tip_to_invalidate->nStatus & BLOCK_FAILED_CHILD) == 0));

    // check all ancestors of the invalidated block are validated up to BLOCK_VALID_TRANSACTIONS and are not invalid
    auto pindex = tip_to_invalidate->pprev;
    while (pindex) {
        WITH_LOCK(::cs_main, assert(pindex->IsValid(BLOCK_VALID_TRANSACTIONS)));
        WITH_LOCK(::cs_main, assert((pindex->nStatus & BLOCK_FAILED_MASK) == 0));
        pindex = pindex->pprev;
    }

    // check all descendants of the invalidated block are BLOCK_FAILED_CHILD
    pindex = orig_tip;
    while (pindex && pindex != tip_to_invalidate) {
        WITH_LOCK(::cs_main, assert((pindex->nStatus & BLOCK_FAILED_VALID) == 0));
        WITH_LOCK(::cs_main, assert(pindex->nStatus & BLOCK_FAILED_CHILD));
        pindex = pindex->pprev;
    }

    // don't mark already invalidated block (orig_tip is BLOCK_FAILED_CHILD) with BLOCK_FAILED_VALID again
    m_node.chainman->ActiveChainstate().InvalidateBlock(state, orig_tip);
    WITH_LOCK(::cs_main, assert(orig_tip->nStatus & BLOCK_FAILED_CHILD));
    WITH_LOCK(::cs_main, assert((orig_tip->nStatus & BLOCK_FAILED_VALID) == 0));
}

BOOST_AUTO_TEST_SUITE_END()
