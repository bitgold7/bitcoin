#!/usr/bin/env python3
"""Ensure blocks enforce dividend fee rules."""
from test_framework.test_framework import BitcoinTestFramework
from test_framework.blocktools import create_block, create_coinbase
from test_framework.p2p import P2PInterface
from test_framework.messages import CTxOut
from test_framework.script import CScript, OP_TRUE


class DividendValidationTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def run_test(self):
        node = self.nodes[0]
        node.p2p = P2PInterface()
        node.p2p.connect_to_node(node)

        tip = int(node.getbestblockhash(), 16)
        height = node.getblockcount() + 1

        # Block missing dividend output
        coinbase = create_coinbase(height)
        block = create_block(tip, coinbase)
        block.solve()
        node.p2p.send_blocks_and_test([block], node, success=False, reject_reason=b"bad-dividend-missing")

        # Block with incorrect dividend amount
        coinbase = create_coinbase(height)
        coinbase.vout.append(CTxOut(0, CScript([OP_TRUE])))
        block = create_block(tip, coinbase)
        block.solve()
        node.p2p.send_blocks_and_test([block], node, success=False, reject_reason=b"bad-dividend-amount")


if __name__ == "__main__":
    DividendValidationTest().main()

