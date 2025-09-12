#!/usr/bin/env python3
"""Test wallet dividend RPCs."""
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

class WalletDividendTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def run_test(self):
        node = self.nodes[0]
        addr = node.getnewaddress()
        info = node.getwalletdividends()
        assert_equal(info, {})
        claimed = node.claimwalletdividends()
        assert_equal(claimed, {})

if __name__ == '__main__':
    WalletDividendTest().main()
