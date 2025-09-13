#!/usr/bin/env python3
"""Test dividend history and next dividend RPCs."""
from decimal import Decimal
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

QUARTER_BLOCKS = 16200

class DividendHistoryTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [["-dividendpayouts=1"]]

    def run_test(self):
        node = self.nodes[0]
        addr = node.getnewaddress()
        node.generatetoaddress(1, addr)
        assert_equal(node.getdividendhistory(), {})
        nxt = node.getnextdividend()
        assert "height" in nxt
        remaining = QUARTER_BLOCKS - node.getblockcount()
        node.generatetoaddress(remaining, addr)
        hist = node.getdividendhistory()
        assert str(QUARTER_BLOCKS) in hist
        whist = node.getwalletdividendhistory()
        assert str(QUARTER_BLOCKS) in whist
        nxt2 = node.getnextdividend()
        assert nxt2["height"] > QUARTER_BLOCKS
        wnxt = node.getwalletnextdividend()
        assert "height" in wnxt

if __name__ == '__main__':
    DividendHistoryTest().main()
