#include <chainparams.h>
#include <consensus/validation.h>
#include <dividend/dividend.h>
#include <consensus/merkle.h>
#include <primitives/block.h>
#include <script/script.h>
#include <optional>
#include <test/util/setup_common.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(coinstake_rewards_tests, TestChain100Setup)

static CBlock BuildBlock(const COutPoint& stake_prevout,
                         CAmount stake_value,
                         CAmount validator_out,
                         const std::optional<std::pair<CAmount, CScript>>& dividend)
{
    CMutableTransaction coinbase;
    coinbase.vin.resize(1);
    coinbase.vin[0].prevout.SetNull();
    coinbase.vout.resize(1);

    CMutableTransaction coinstake;
    coinstake.vin.emplace_back(stake_prevout);
    coinstake.vout.resize(dividend ? 3 : 2);
    coinstake.vout[0].SetNull();
    coinstake.vout[1].nValue = stake_value + validator_out;
    if (dividend) {
        coinstake.vout[2].nValue = dividend->first;
        coinstake.vout[2].scriptPubKey = dividend->second;
    }

    CBlock block;
    block.vtx.emplace_back(MakeTransactionRef(std::move(coinbase)));
    block.vtx.emplace_back(MakeTransactionRef(std::move(coinstake)));
    block.hashPrevBlock = Assert(m_node.chainman)->ActiveChain().Tip()->GetBlockHash();
    block.nTime = Assert(m_node.chainman)->ActiveChain().Tip()->GetBlockTime() + 16;
    block.hashMerkleRoot = BlockMerkleRoot(block);
    return block;
}

BOOST_AUTO_TEST_CASE(invalid_validator_only_reward)
{
    Consensus::Params params = Params().GetConsensus();
    params.fEnablePoS = false; // bypass contextual PoS checks

    const CTransactionRef& stake_coin = m_coinbase_txns[0];
    COutPoint prevout{stake_coin->GetHash(), 0};
    CAmount stake_value = stake_coin->vout[0].nValue;

    int height = Assert(m_node.chainman)->ActiveChain().Height() + 1;
    CAmount subsidy = GetBlockSubsidy(height, params);

    CBlock block = BuildBlock(prevout, stake_value, subsidy, std::nullopt);

    BlockValidationState state;
    BOOST_CHECK(!CheckBlock(block, state, params));
    BOOST_CHECK_EQUAL(state.GetRejectReason(), "bad-validator-amount");
}

BOOST_AUTO_TEST_CASE(invalid_dividend_script)
{
    Consensus::Params params = Params().GetConsensus();
    params.fEnablePoS = false;

    const CTransactionRef& stake_coin = m_coinbase_txns[0];
    COutPoint prevout{stake_coin->GetHash(), 0};
    CAmount stake_value = stake_coin->vout[0].nValue;

    int height = Assert(m_node.chainman)->ActiveChain().Height() + 1;
    CAmount subsidy = GetBlockSubsidy(height, params);
    CAmount dividend_reward = subsidy / 10;
    CAmount validator_reward = subsidy - dividend_reward;

    CScript wrong_script = CScript() << OP_TRUE;
    CBlock block = BuildBlock(prevout, stake_value, validator_reward,
                              std::make_pair(dividend_reward, wrong_script));

    BlockValidationState state;
    BOOST_CHECK(!CheckBlock(block, state, params));
    BOOST_CHECK_EQUAL(state.GetRejectReason(), "bad-dividend-script");
}

BOOST_AUTO_TEST_SUITE_END()

