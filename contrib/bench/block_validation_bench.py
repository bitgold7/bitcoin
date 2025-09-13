#!/usr/bin/env python3
# Copyright (c) 2024 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""Benchmark validation of ~20 MB blocks on regtest."""

import os
import sys
import time

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'test', 'functional'))

from test_framework.test_framework import BitcoinTestFramework
from test_framework.wallet import MiniWallet
from test_framework.util import create_lots_of_big_transactions, gen_return_txouts


class BlockValidationBench(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        target_weight = 20_000_000
        self.extra_args = [
            [f"-blockmaxweight={target_weight}"],
            [f"-blockmaxweight={target_weight}"],
        ]

    def run_test(self):
        node0, node1 = self.nodes
        mini_wallet = MiniWallet(node0)
        mini_wallet.generate(500)
        self.sync_blocks()

        txouts = gen_return_txouts()
        fee = 100 * node0.getnetworkinfo()["relayfee"]
        approx_tx_size = 200 + sum(len(txout.serialize()) for txout in txouts)
        tx_count = int(20_000_000 // approx_tx_size + 1)
        self.log.info(f"Creating {tx_count} large transactions")
        create_lots_of_big_transactions(mini_wallet, node0, fee, tx_count, txouts)

        self.disconnect_nodes(0, 1)
        block_hash = self.generate(node0, 1)[0]
        block_hex = node0.getblock(block_hash, 0)

        start = time.time()
        node1.submitblock(block_hex)
        elapsed = time.time() - start
        self.log.info(f"Validation time: {elapsed:.2f}s")


if __name__ == "__main__":
    BlockValidationBench().main()
