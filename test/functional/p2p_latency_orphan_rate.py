#!/usr/bin/env python3
# Copyright (c) 2024 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import time

from test_framework.test_framework import BitcoinTestFramework
from test_framework.p2p import P2PInterface
from test_framework.wallet import MiniWallet
from test_framework.messages import msg_tx
from test_framework.util import assert_less_than

class LatencyOrphanRateTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [[]]

    def run_test(self):
        node = self.nodes[0]
        wallet = MiniWallet(node)
        self.generate(node, 101)
        peer = node.add_p2p_connection(P2PInterface())

        pairs = []
        for _ in range(5):
            parent = wallet.create_self_transfer()
            child = wallet.create_self_transfer(utxo_to_spend=parent["new_utxo"])
            pairs.append((parent["tx"], child["tx"]))

        for parent_tx, child_tx in pairs:
            peer.send_message(msg_tx(child_tx))
            time.sleep(0.1)

        self.wait_until(lambda: len(node.getorphantxs()) == len(pairs))
        assert_less_than(len(node.getorphantxs()), 10)

        for parent_tx, child_tx in pairs:
            peer.send_message(msg_tx(parent_tx))

        self.wait_until(lambda: len(node.getorphantxs()) == 0)

if __name__ == '__main__':
    LatencyOrphanRateTest(__file__).main()
