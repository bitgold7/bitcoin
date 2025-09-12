#include <consensus/amount.h>
#include <policy/hybrid_comparator.h>
#include <test/util/txmempool.h>
#include <util/time.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(hybrid_comparator_tests)

BOOST_AUTO_TEST_CASE(prioritizes_fee_age_stake)
{
    SetMockTime(3);

    // Base transaction setup
    CMutableTransaction base_tx;
    base_tx.vin.resize(1);
    base_tx.vout.resize(1);
    base_tx.vout[0].nValue = 1 * COIN;

    // Fee prioritization
    CTxMemPoolEntry fee_high = TestMemPoolEntryHelper().Fee(2000).Time(NodeSeconds{1}).FromTx(base_tx);
    CTxMemPoolEntry fee_low = TestMemPoolEntryHelper().Fee(1000).Time(NodeSeconds{1}).FromTx(base_tx);
    HybridComparator cmp;
    BOOST_CHECK(cmp(fee_high, fee_low));

    // Age prioritization (fees equal)
    CTxMemPoolEntry age_old = TestMemPoolEntryHelper().Fee(1000).Time(NodeSeconds{1}).FromTx(base_tx);
    CTxMemPoolEntry age_new = TestMemPoolEntryHelper().Fee(1000).Time(NodeSeconds{2}).FromTx(base_tx);
    BOOST_CHECK(cmp(age_old, age_new));

    // Stake prioritization (fees and age equal)
    CMutableTransaction stake_high_tx = base_tx;
    stake_high_tx.vout[0].nValue = 2 * COIN;
    CMutableTransaction stake_low_tx = base_tx;
    stake_low_tx.vout[0].nValue = 1 * COIN;
    CTxMemPoolEntry stake_high = TestMemPoolEntryHelper().Fee(1000).Time(NodeSeconds{1}).FromTx(stake_high_tx);
    CTxMemPoolEntry stake_low = TestMemPoolEntryHelper().Fee(1000).Time(NodeSeconds{1}).FromTx(stake_low_tx);
    BOOST_CHECK(cmp(stake_high, stake_low));

    SetMockTime(0);
}

BOOST_AUTO_TEST_SUITE_END()
