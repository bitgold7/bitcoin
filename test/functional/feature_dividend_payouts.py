#!/usr/bin/env python3
"""Test dividend payouts across a quarter boundary."""
from decimal import Decimal
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

class DividendPayoutsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.extra_args = [["-dividendpayouts=1"]]

    def run_test(self):
        node = self.nodes[0]
        quarter = 16200

        node.generate(100)
        info = node.getdividendinfo()
        assert info["pool"] > 0

        height = node.getblockcount()
        node.generate(quarter - height)

        info2 = node.getdividendinfo()
        assert_equal(info2["pool"], Decimal("0"))
        assert str(quarter) in info2["snapshots"]

        addr = node.getnewaddress()
        claim = node.claimdividends(addr)
        assert_equal(claim["claimed"], Decimal("0"))

if __name__ == '__main__':
    DividendPayoutsTest(__file__).main()
