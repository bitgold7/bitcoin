#include <policy/priority.h>
#include <coins.h>
#include <kernel/mempool_priority.h>
#include <txmempool.h>
#include <util/translation.h>
#include <test/util/setup_common.h>
#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(priority_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(stake_duration_uses_pos_spacing)
{
    // Create an empty mempool
    bilingual_str error;
    CTxMemPool pool{CTxMemPool::Options{}, error};

    // Setup coins view with a single coin aged n_blocks
    CCoinsView base;
    CCoinsViewCache view(&base);
    const CAmount value{1000 * COIN};
    const int chain_height{200};
    const int coin_height{100};
    const int n_blocks{chain_height - coin_height};
    COutPoint prevout{Txid::FromUint256(uint256{1}), 0};
    view.AddCoin(prevout, Coin(CTxOut(value, CScript{}), coin_height, /*coinbase=*/false), false);

    // Transaction spending the coin
    CMutableTransaction mtx;
    mtx.vin.emplace_back(prevout);
    mtx.vout.emplace_back(value, CScript{});
    const CTransaction tx{mtx};

    // Consensus params with differing PoW and PoS spacing
    Consensus::Params params = Params().GetConsensus();
    params.nPowTargetSpacing = 6000;   // Below 7-day threshold when multiplied
    params.nStakeTargetSpacing = 7000; // Above 7-day threshold when multiplied

    const int64_t expected_duration = int64_t(n_blocks) * params.nStakeTargetSpacing;
    const int64_t expected = CalculateStakePriority(tx.GetValueOut()) +
                             CalculateStakeDurationPriority(expected_duration);

    const int64_t actual = GetBGDPriority(tx, /*fee=*/0, view, pool,
                                          chain_height, params,
                                          /*signals_rbf=*/false, /*has_ancestors=*/false);

    BOOST_CHECK_EQUAL(expected, actual);
}

BOOST_AUTO_TEST_SUITE_END()
