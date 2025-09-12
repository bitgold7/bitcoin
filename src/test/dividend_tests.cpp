#include <dividend/dividend.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(dividend_tests)

BOOST_AUTO_TEST_CASE(multi_stakeholder_snapshots_and_no_double_payout)
{
    using namespace dividend;
    std::map<std::string, StakeInfo> stakes{
        {"Alice", {500 * COIN, 0}},
        {"Bob", {1000 * COIN, QUARTER_BLOCKS / 2}},
    };
    const int height = QUARTER_BLOCKS;
    const CAmount pool = 1000 * COIN;
    auto payouts = CalculatePayouts(stakes, height, pool);
    BOOST_CHECK_EQUAL(payouts.size(), 2U);
    BOOST_CHECK(payouts.find("Alice") != payouts.end());
    BOOST_CHECK(payouts.find("Bob") != payouts.end());
    // Update snapshot to prevent double payouts
    for (auto& [addr, info] : stakes) {
        info.last_payout_height = height;
    }
    auto none = CalculatePayouts(stakes, height, pool);
    BOOST_CHECK(none.empty());
}

BOOST_AUTO_TEST_SUITE_END()
