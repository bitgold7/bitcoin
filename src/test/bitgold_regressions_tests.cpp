#include <test/data/bitgold_regressions.json.h>
#include <test/util/setup_common.h>
#include <test/util/json.h>

#include <core_io.h>
#include <consensus/tx_check.h>
#include <consensus/validation.h>
#include <validation.h>
#include <chainparams.h>

#include <boost/test/unit_test.hpp>

#include <univalue.h>

BOOST_FIXTURE_TEST_SUITE(bitgold_regressions_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(replay_vectors)
{
    UniValue tests = read_json(json_tests::bitgold_regressions);
    for (unsigned int i = 0; i < tests.size(); ++i) {
        const UniValue& test = tests[i];
        std::string desc = test["description"].get_str();
        if (test.exists("tx")) {
            CMutableTransaction mtx;
            BOOST_CHECK_MESSAGE(DecodeHexTx(mtx, test["tx"].get_str()), desc + ": decode tx");
            CTransaction tx(mtx);
            TxValidationState state;
            BOOST_CHECK_MESSAGE(!CheckTransaction(tx, state), desc);
        } else if (test.exists("block")) {
            CBlock block;
            BOOST_CHECK_MESSAGE(DecodeHexBlock(block, test["block"].get_str()), desc + ": decode block");
            BlockValidationState state;
            BOOST_CHECK_MESSAGE(!CheckBlock(block, state, Params().GetConsensus()), desc);
        } else {
            BOOST_ERROR("Test vector missing tx or block: " << desc);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
