// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <chainparams.h>
#include <consensus/amount.h>
#include <test/util/mining.h>
#include <test/util/setup_common.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(subsidy_tests, RegTestingSetup)

BOOST_AUTO_TEST_CASE(block1_reward_and_total_subsidy)
{
    // Mine block 1 and check its subsidy
    auto blocks = CreateBlockChain(1, Params());
    std::shared_ptr<CBlock> block1 = blocks.at(0);
    BOOST_REQUIRE(!ProcessBlock(m_node, block1).IsNull());
    BOOST_CHECK_EQUAL(block1->vtx[0]->vout[0].nValue, 3'000'000 * COIN);

    // Sum subsidies until exhaustion using main network parameters
    const auto main_params = CreateChainParams(*m_node.args, ChainType::MAIN);
    const Consensus::Params& consensus = main_params->GetConsensus();
    CAmount total{0};
    for (int height = 1;; ++height) {
        CAmount subsidy = GetBlockSubsidy(height, consensus);
        if (subsidy == 0) break;
        total += subsidy;
    }
    BOOST_CHECK_EQUAL(total, 8'000'000 * COIN);
}

BOOST_AUTO_TEST_SUITE_END()
