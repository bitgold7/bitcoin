#!/usr/bin/env python3
# Copyright (c) 2024 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test default pruning by syncing ~100k blocks.

The node is started with -prune (default target). After mining around
100k blocks and additional large blocks to exceed the target, older
blocks should be pruned automatically.
"""
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_raises_rpc_error
from test_framework.wallet import MiniWallet


class FeaturePruneDefaultTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [["-prune"]]

    def mine_in_batches(self, generator, blocks):
        for _ in range(blocks // 1000):
            self.generate(generator, 1000, sync_fun=self.no_op)
        if blocks % 1000:
            self.generate(generator, blocks % 1000, sync_fun=self.no_op)

    def run_test(self):
        wallet = MiniWallet(self.nodes[0])

        # Mine some blocks to the wallet for spendable outputs and let them mature
        self.mine_in_batches(wallet, 101)
        self.mine_in_batches(self.nodes[0], 99899)

        old_block = self.nodes[0].getblockhash(1)

        # Create a large witness to grow block files quickly (~1 MB per block)
        annex = b"\x50" + b"\xff" * 1_000_000

        for _ in range(600):
            tx = wallet.create_self_transfer()["tx"]
            tx.wit.vtxinwit[0].scriptWitness.stack.append(annex)
            self.generateblock(self.nodes[0], transactions=[tx.serialize().hex()], sync_fun=self.no_op)
            wallet.scan_tx(self.nodes[0].decoderawtransaction(tx.serialize().hex()))

        self.wait_until(lambda: self.nodes[0].getblockchaininfo()["pruneheight"] > 0)
        assert_raises_rpc_error(-32100, "Block not available (pruned data)", self.nodes[0].getblock, old_block)


if __name__ == '__main__':
    FeaturePruneDefaultTest(__file__).main()
