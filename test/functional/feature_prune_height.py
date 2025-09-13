#!/usr/bin/env python3
"""Verify pruning occurs once the chain passes the configured height."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_greater_than_or_equal, assert_raises_rpc_error
from test_framework.wallet import MiniWallet

class FeaturePruneHeightTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [["-fastprune", "-prune=1"]]

    def run_test(self):
        node = self.nodes[0]
        wallet = MiniWallet(node)
        prune_after_height = 100  # Regtest prune height when -fastprune is enabled

        # Mine enough blocks so that pruning can take place once the target is
        # reached: nPruneAfterHeight + MIN_BLOCKS_TO_KEEP + 1
        self.generate(wallet, 101)  # make mature coins for transactions
        self.generate(node, prune_after_height + 288 - 101, sync_fun=self.no_op)

        old_block = node.getblockhash(1)

        # Create large blocks to exceed the prune target quickly
        annex = b"\x50" + b"\xff" * 1_000_000
        for _ in range(2):
            tx = wallet.create_self_transfer()["tx"]
            tx.wit.vtxinwit[0].scriptWitness.stack.append(annex)
            self.generateblock(node, [tx.serialize().hex()], sync_fun=self.no_op)
            wallet.scan_tx(node.decoderawtransaction(tx.serialize().hex()))

        self.wait_until(lambda: node.getblockchaininfo()["pruneheight"] > 0)
        info = node.getblockchaininfo()
        assert_greater_than_or_equal(info["pruneheight"], prune_after_height)
        assert_raises_rpc_error(-32100, "Block not available (pruned data)",
                               node.getblock, old_block)

if __name__ == '__main__':
    FeaturePruneHeightTest(__file__).main()
