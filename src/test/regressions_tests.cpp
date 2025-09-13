#include <test/data/regressions/invalid_tx_extra_junk.hex.h>
#include <test/util/setup_common.h>

#include <consensus/tx_check.h>
#include <consensus/validation.h>
#include <core_io.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(regressions_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(invalid_tx_extra_junk)
{
    const auto& data = test::data::regressions::invalid_tx_extra_junk;
    std::string hex;
    hex.reserve(data.size());
    for (std::byte b : data)
        hex.push_back(static_cast<char>(b));

    CMutableTransaction mtx;
    BOOST_CHECK(DecodeHexTx(mtx, hex));
    const CTransaction tx{mtx};
    TxValidationState state;
    BOOST_CHECK(!CheckTransaction(tx, state));
}

BOOST_AUTO_TEST_SUITE_END()
