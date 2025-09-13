#!/usr/bin/env python3
"""Test wallet balance after unblinding confidential transactions."""
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

class WalletBlindingTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [[]]

    def run_test(self):
        node = self.nodes[0]
        node.generate(101)
        blinded = node.getnewshieldedaddress()
        node.sendshielded(blinded, 1)
        node.generate(1)
        assert_equal(node.getbalance(), 0)
        node.rescanblockchain()
        assert_equal(node.getbalance(), 1)

if __name__ == '__main__':
    WalletBlindingTest(__file__).main()
