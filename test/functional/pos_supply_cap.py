#!/usr/bin/env python3
"""Verify staking supply cap and halving schedule over 100k blocks."""

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
        for _ in range(100):
            node.generatetoaddress(1000, addr)
        node.generatetoaddress(1, addr)

        # Check subsidy before and after the halving boundaries at heights
        # 50_000 and 100_000
        subsidy_50k = node.getblockstats(50_000)["subsidy"]
        subsidy_50k1 = node.getblockstats(50_001)["subsidy"]
        subsidy_100k = node.getblockstats(100_000)["subsidy"]
        subsidy_100k1 = node.getblockstats(100_001)["subsidy"]
        assert_equal(subsidy_50k, 50 * COIN)
        assert_equal(subsidy_50k1, 25 * COIN)
        assert_equal(subsidy_100k, 25 * COIN)
        assert_equal(subsidy_100k1, 25 * COIN // 2)

        # Verify total minted coins including genesis do not exceed 8M
        utxo_info = node.gettxoutsetinfo()
        total_amount = utxo_info["total_amount"]
        expected = Decimal(3_000_000 + 50_000 * 50 + 50_000 * 25) + Decimal("12.5")
        assert_equal(total_amount, expected)
        assert total_amount <= Decimal(8_000_000)

if __name__ == '__main__':
    PosSupplyCapTest(__file__).main()
