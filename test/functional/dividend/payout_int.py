#!/usr/bin/env python3
"""Verify dividend payouts use integer arithmetic."""
from decimal import Decimal
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

QUARTER_BLOCKS = 16200

class DividendIntegerPayoutTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [["-dividendpayouts=1"]]

    def run_test(self):
        node = self.nodes[0]
        stakes = [
            {"address": "a", "weight": Decimal("1"), "last_payout_height": 0},
            {"address": "b", "weight": Decimal("2"), "last_payout_height": 0},
        ]
        # Pool smaller than desired to exercise scaling logic
        payouts = node.getdividendschedule(stakes, QUARTER_BLOCKS, Decimal("0.01"))
        assert_equal(payouts["a"], Decimal("0.00333333"))
        assert_equal(payouts["b"], Decimal("0.00666666"))

if __name__ == '__main__':
    DividendIntegerPayoutTest().main()
