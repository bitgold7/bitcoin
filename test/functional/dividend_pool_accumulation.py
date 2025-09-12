#!/usr/bin/env python3
"""Test dividend pool accumulation and payout snapshots."""
from decimal import Decimal
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

QUARTER_BLOCKS = 16200

class DividendPoolAccumulationTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.extra_args = [["-dividendpayouts=1"]]

    def run_test(self):
        node = self.nodes[0]
        addr = node.getnewaddress()

        node.generatetoaddress(1, addr)
        pool = node.getdividendpool()
        assert "amount" in pool
        assert pool["amount"] > Decimal("0")

        # Generate up to the end of the quarter so payouts are processed
        remaining = QUARTER_BLOCKS - node.getblockcount()
        node.generatetoaddress(remaining, addr)
        pool_after = node.getdividendpool()
        assert_equal(pool_after["amount"], Decimal("0"))

        pending = node.getpendingdividends()
        assert isinstance(pending, dict)
        snaps = node.getstakesnapshots()
        assert len(snaps) == 1
        snap_hash = list(snaps.keys())[0]
        assert_equal(len(snap_hash), 64)

        # Claiming from address should succeed even if amount is zero
        claim = node.claimdividends(addr)
        assert "claimed" in claim

if __name__ == '__main__':
    DividendPoolAccumulationTest().main()
