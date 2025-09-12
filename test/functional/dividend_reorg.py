#!/usr/bin/env python3
"""Ensure dividend payouts remain stable across deep reorgs."""
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

QUARTER_BLOCKS = 16200

class DividendReorgTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True
        self.extra_args = [["-dividendpayouts=1"], ["-dividendpayouts=1"]]

    def run_test(self):
        node0, node1 = self.nodes
        addr0 = node0.getnewaddress()

        node0.generatetoaddress(110, addr0)
        self.sync_all()

        node0.startstaking()
        node1.startstaking()
        node0.sendtoaddress(addr0, 1)
        fork_point = node0.getblockcount()
        node0.waitforblockheight(fork_point + 1)
        self.sync_all()

        # Split network and build competing chains
        self.disconnect_nodes(0, 1)
        remaining = QUARTER_BLOCKS - node0.getblockcount()
        node0.generatetoaddress(remaining, addr0)
        payout_before = node0.getpendingdividends()[addr0]
        node1.generatetoaddress(remaining + 5, addr0)
        bad_block = node1.getblockhash(fork_point + 1)

        # Reorg to node1's longer chain
        self.connect_nodes(0, 1)
        self.sync_all()
        assert_equal(node0.getblockcount(), node1.getblockcount())

        # Return to node0's original chain and extend it
        self.disconnect_nodes(0, 1)
        node0.invalidateblock(bad_block)
        node0.generatetoaddress(10, addr0)
        payout_after = node0.getpendingdividends()[addr0]
        assert_equal(payout_before, payout_after)

        # Final reorg to node0's chain
        self.connect_nodes(0, 1)
        self.sync_all()
        assert_equal(node0.getblockcount(), node1.getblockcount())
        assert_equal(node1.getpendingdividends()[addr0], payout_before)

if __name__ == '__main__':
    DividendReorgTest().main()
