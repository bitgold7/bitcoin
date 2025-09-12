#include <boost/test/unit_test.hpp>
#include <dividend/dividend.h>
#include <test/util/setup_common.h>

BOOST_FIXTURE_TEST_SUITE(dividend_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(payouts_basic)
{
    using namespace dividend;
    std::map<std::string, StakeInfo> stakes{
        {"A", StakeInfo{10 * COIN, 0}},
        {"B", StakeInfo{20 * COIN, 0}},
    };
    CAmount pool{100 * COIN};
    auto payouts{CalculatePayouts(stakes, QUARTER_BLOCKS, pool)};
    BOOST_CHECK_EQUAL(payouts.size(), 2);
    BOOST_CHECK_EQUAL(payouts["A"], 8125000);
    BOOST_CHECK_EQUAL(payouts["B"], 16250000);
}

BOOST_AUTO_TEST_CASE(payouts_scaled)
{
    using namespace dividend;
    std::map<std::string, StakeInfo> stakes{
        {"A", StakeInfo{10 * COIN, 0}},
        {"B", StakeInfo{20 * COIN, 0}},
    };
    CAmount pool{static_cast<CAmount>(0.1 * COIN)};
    auto payouts{CalculatePayouts(stakes, QUARTER_BLOCKS, pool)};
    BOOST_CHECK_EQUAL(payouts["B"], 2 * payouts["A"]);
    BOOST_CHECK_LE(std::abs(payouts["A"] + payouts["B"] - pool), 1);
}

BOOST_AUTO_TEST_CASE(no_payouts)
{
    using namespace dividend;
    std::map<std::string, StakeInfo> stakes{
        {"A", StakeInfo{10 * COIN, QUARTER_BLOCKS}},
    };
    auto payouts{CalculatePayouts(stakes, QUARTER_BLOCKS, 100 * COIN)};
    BOOST_CHECK(payouts.empty());
    payouts = CalculatePayouts(stakes, QUARTER_BLOCKS - 1, 100 * COIN);
    BOOST_CHECK(payouts.empty());
    payouts = CalculatePayouts(stakes, QUARTER_BLOCKS, 0);
    BOOST_CHECK(payouts.empty());
}

BOOST_AUTO_TEST_SUITE_END()
