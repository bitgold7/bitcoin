#include <chainparams.h>
#include <consensus/amount.h>
#include <test/util/setup_common.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(monetary_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(block_subsidies)
{
    const auto chainparams = CreateChainParams(*m_node.args, ChainType::MAIN);
    const auto& consensus = chainparams->GetConsensus();

    BOOST_CHECK_EQUAL(GetBlockSubsidy(1, consensus), 3'000'000 * COIN);
    BOOST_CHECK_EQUAL(GetBlockSubsidy(2, consensus), 50 * COIN);
    BOOST_CHECK_EQUAL(GetBlockSubsidy(2 + consensus.nSubsidyHalvingInterval - 1, consensus), 50 * COIN);
    BOOST_CHECK_EQUAL(GetBlockSubsidy(2 + consensus.nSubsidyHalvingInterval, consensus), 25 * COIN);
}

BOOST_AUTO_TEST_CASE(total_supply_limit)
{
    const auto chainparams = CreateChainParams(*m_node.args, ChainType::MAIN);
    const auto& consensus = chainparams->GetConsensus();

    CAmount sum{50 * COIN}; // genesis subsidy
    for (int height = 1;; ++height) {
        CAmount subsidy = GetBlockSubsidy(height, consensus);
        if (subsidy == 0) break;
        sum += subsidy;
    }
    BOOST_CHECK_EQUAL(sum, CAmount{8'000'000 * COIN});
}

BOOST_AUTO_TEST_SUITE_END()
