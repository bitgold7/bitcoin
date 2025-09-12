#include <boost/test/unit_test.hpp>
#include <dividend/dividend.h>
#include <primitives/transaction.h>
#include <test/util/setup_common.h>
#include <vector>

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

BOOST_AUTO_TEST_CASE(multi_stakeholders)
{
    using namespace dividend;
    std::map<std::string, StakeInfo> stakes{
        {"A", StakeInfo{10 * COIN, 0}},
        {"B", StakeInfo{20 * COIN, QUARTER_BLOCKS}},
        {"C", StakeInfo{40 * COIN, 0}},
    };
    CAmount pool{100 * COIN};
    auto payouts{CalculatePayouts(stakes, 2 * QUARTER_BLOCKS, pool)};
    BOOST_CHECK_EQUAL(payouts.size(), 3);
    BOOST_CHECK(payouts.count("A"));
    BOOST_CHECK(payouts.count("B"));
    BOOST_CHECK(payouts.count("C"));
}

BOOST_AUTO_TEST_CASE(double_payout_prevention)
{
    using namespace dividend;
    std::map<std::string, StakeInfo> stakes{{"A", StakeInfo{10 * COIN, 0}}};
    CAmount pool{100 * COIN};
    auto payouts{CalculatePayouts(stakes, QUARTER_BLOCKS, pool)};
    BOOST_CHECK(!payouts.empty());
    stakes["A"].last_payout_height = QUARTER_BLOCKS;
    payouts = CalculatePayouts(stakes, QUARTER_BLOCKS, pool);
    BOOST_CHECK(payouts.empty());
}

BOOST_AUTO_TEST_CASE(rounding_behavior)
{
    using namespace dividend;
    std::map<std::string, StakeInfo> stakes{{"A", {10 * COIN, 0}}, {"B", {20 * COIN, 0}}};
    CAmount pool{100};
    auto expected = CalculatePayouts(stakes, QUARTER_BLOCKS, pool);
    CMutableTransaction tx = BuildPayoutTx(stakes, QUARTER_BLOCKS, pool);
    std::vector<std::pair<std::string, CAmount>> exp_vec(expected.begin(), expected.end());
    CAmount paid{0};
    for (size_t i = 0; i < exp_vec.size(); ++i) {
        BOOST_CHECK_EQUAL(tx.vout[i].nValue, exp_vec[i].second);
        paid += exp_vec[i].second;
    }
    CAmount leftover = pool - paid;
    if (leftover > 0) {
        BOOST_CHECK_EQUAL(tx.vout.back().nValue, leftover);
    }
    CAmount total{0};
    for (const auto& out : tx.vout) total += out.nValue;
    BOOST_CHECK_EQUAL(total, pool);
}

BOOST_AUTO_TEST_CASE(chain_reorg_consistency)
{
    using namespace dividend;
    std::map<std::string, StakeInfo> stakes{{"A", {15 * COIN, 0}}, {"B", {5 * COIN, 0}}};
    CAmount pool{50 * COIN};
    CMutableTransaction tx1 = BuildPayoutTx(stakes, QUARTER_BLOCKS, pool);
    CMutableTransaction tx2 = BuildPayoutTx(stakes, QUARTER_BLOCKS, pool);
    BOOST_CHECK_EQUAL(CTransaction(tx1).GetHash(), CTransaction(tx2).GetHash());
}

BOOST_AUTO_TEST_CASE(empty_pool)
{
    using namespace dividend;
    std::map<std::string, StakeInfo> stakes{{"A", {10 * COIN, 0}}, {"B", {20 * COIN, 0}}};
    auto payouts1 = CalculatePayouts(stakes, QUARTER_BLOCKS, 0);
    auto payouts2 = CalculatePayouts(stakes, QUARTER_BLOCKS, 0);
    BOOST_CHECK(payouts1.empty());
    BOOST_CHECK(payouts1 == payouts2);
    CMutableTransaction tx = BuildPayoutTx(stakes, QUARTER_BLOCKS, 0);
    BOOST_CHECK(tx.vout.empty());
}

BOOST_AUTO_TEST_SUITE_END()
