#!/usr/bin/env python3
"""Verify staking supply cap and halving schedule."""

from decimal import Decimal

from test_framework.test_framework import BitcoinTestFramework
from test_framework.messages import COIN
from test_framework.util import assert_equal

class PosSupplyCapTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.rpc_timeout = 120

    def run_test(self):
        node = self.nodes[0]
        addr = node.getnewaddress()
        for _ in range(50):
            node.generatetoaddress(1000, addr)
        node.generatetoaddress(1, addr)

        # Check subsidy before and after the halving boundary at height 50_000
        subsidy_before = node.getblockstats(50_000)["subsidy"]
        subsidy_after = node.getblockstats(50_001)["subsidy"]
        assert_equal(subsidy_before, 50 * COIN)
        assert_equal(subsidy_after, 25 * COIN)

        # Verify total minted coins including genesis do not exceed 8M
        utxo_info = node.gettxoutsetinfo()
        total_amount = utxo_info["total_amount"]
        expected = Decimal(3_000_000 + 50_000 * 50 + 25)
        assert_equal(total_amount, expected)
        assert total_amount <= Decimal(8_000_000)

if __name__ == '__main__':
    PosSupplyCapTest(__file__).main()
