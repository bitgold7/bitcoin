#include <consensus/amount.h>
#include <policy/policy.h>
#include <script/script.h>
#include <txmempool.h>

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(txmempool_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(hybrid_score_prioritizes_stake_over_fee)
{
    // Two transactions with similar fee band but different stake weight
    CMutableTransaction mtx1;
    mtx1.vin.emplace_back();
    mtx1.vout.emplace_back(1 * COIN, CScript{});
    const CTransactionRef tx1{MakeTransactionRef(mtx1)};

    CMutableTransaction mtx2;
    mtx2.vin.emplace_back();
    mtx2.vout.emplace_back(2 * COIN, CScript{});
    const CTransactionRef tx2{MakeTransactionRef(mtx2)};

    LockPoints lp{};
    CTxMemPoolEntry e1{tx1, /*fee=*/1500, /*time=*/0, /*height=*/1, /*sequence=*/0, /*spends_coinbase=*/false, /*sigops=*/1, lp};
    CTxMemPoolEntry e2{tx2, /*fee=*/1400, /*time=*/0, /*height=*/1, /*sequence=*/0, /*spends_coinbase=*/false, /*sigops=*/1, lp};

    // Pure fee-rate ordering prefers e1.
    g_hybrid_mempool = false;
    BOOST_CHECK(CompareTxMemPoolEntryByScore()(e1, e2));

    // Hybrid ordering prefers e2 due to larger stake weight.
    g_hybrid_mempool = true;
    BOOST_CHECK(!CompareTxMemPoolEntryByScore()(e1, e2));

    g_hybrid_mempool = false;
}

BOOST_AUTO_TEST_SUITE_END()

