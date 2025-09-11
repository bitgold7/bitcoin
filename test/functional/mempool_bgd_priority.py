#!/usr/bin/env python3
# Copyright (c) 2024 The Bitcoin Core developers
# Distributed under the MIT software license.
"""Test BGD mempool priority logic"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.wallet import MiniWallet
from test_framework.util import assert_greater_than_or_equal, assert_equal

class MempoolBGDPriorityTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [["-bgdpriority", "-maxmempool=1"]]

    def run_test(self):
        node = self.nodes[0]
        wallet = MiniWallet(node)
        self.generate(node, 101)
        wallet.rescan_utxos()

        # Create two large transactions, one signaling RBF (low priority) and one non-RBF
        tx_keep = wallet.create_self_transfer(target_vsize=100000, sequence=0xFFFFFFFF)
        tx_keep_id = wallet.sendrawtransaction(node, tx_keep["hex"])
        tx_rbf = wallet.create_self_transfer(target_vsize=100000, sequence=0)
        tx_rbf_id = wallet.sendrawtransaction(node, tx_rbf["hex"])

        pr_keep = node.getmempoolentry(tx_keep_id)["priority"]
        pr_rbf = node.getmempoolentry(tx_rbf_id)["priority"]
        assert pr_keep > pr_rbf

        # Fill mempool to trigger eviction; the RBF tx should be evicted first
        for _ in range(8):
            tx = wallet.create_self_transfer(target_vsize=100000, sequence=0xFFFFFFFF)
            wallet.sendrawtransaction(node, tx["hex"])
        extra = wallet.create_self_transfer(target_vsize=100000, sequence=0xFFFFFFFF)
        wallet.sendrawtransaction(node, extra["hex"])
        mempool = node.getrawmempool()
        assert tx_keep_id in mempool
        assert tx_rbf_id not in mempool

        # Mine a block to clear mempool
        self.generate(node, 1)
        wallet.rescan_utxos()

        # CPFP: child should have higher priority than parent by bonus amount
        parent = wallet.create_self_transfer(sequence=0xFFFFFFFF)
        parent_id = wallet.sendrawtransaction(node, parent["hex"])
        child = wallet.create_self_transfer(utxo_to_spend=parent["new_utxo"], sequence=0xFFFFFFFF)
        child_id = wallet.sendrawtransaction(node, child["hex"])
        parent_prio = node.getmempoolentry(parent_id)["priority"]
        child_prio = node.getmempoolentry(child_id)["priority"]
        assert_greater_than_or_equal(child_prio, parent_prio + 5)

        # Admission: transaction accepted with priority value present
        entry = node.getmempoolentry(child_id)
        assert "priority" in entry

if __name__ == '__main__':
    MempoolBGDPriorityTest().main()
