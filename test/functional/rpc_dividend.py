#!/usr/bin/env python3
"""Test getdividendinfo and getdividendhistory RPCs."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

QUARTER_BLOCKS = 16200


class RpcDividendTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [["-dividendpayouts=1"]]

    def run_test(self):
        node = self.nodes[0]
        addr = node.getnewaddress()
        node.generatetoaddress(1, addr)

        info = node.getdividendinfo()
        assert "pool" in info
        assert "next_height" in info
        assert "payouts" in info

        assert_equal(node.getdividendhistory(), {})

        remaining = QUARTER_BLOCKS - node.getblockcount()
        node.generatetoaddress(remaining, addr)

        hist = node.getdividendhistory()
        assert str(QUARTER_BLOCKS) in hist

        info2 = node.getdividendinfo()
        assert info2["next_height"] > QUARTER_BLOCKS


if __name__ == '__main__':
    RpcDividendTest(__file__).main()

