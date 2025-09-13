#include <chainparams.h>
#include <consensus/amount.h>
#include <test/util/setup_common.h>
#include <util/chaintype.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(reward_schedule_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(total_supply_below_cap)
{
    const auto params = CreateChainParams(*m_node.args, ChainType::MAIN);
    const auto& consensus = params->GetConsensus();

    CAmount total{0};
    for (int height = 0;; ++height) {
        const CAmount subsidy{GetBlockSubsidy(height, consensus)};
        if (subsidy == 0) break;
        total += subsidy;
    }
    BOOST_CHECK_LE(total, 8'000'000 * COIN);
}

BOOST_AUTO_TEST_SUITE_END()
