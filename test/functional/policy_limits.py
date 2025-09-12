#!/usr/bin/env python3
# Copyright (c) 2024 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test various policy limits: mempool size cap, block weight and sigops."""

from test_framework.blocktools import (
    MAX_BLOCK_SIGOPS_WEIGHT,
    create_block,
    create_coinbase,
    get_legacy_sigopcount_block,
)
from test_framework.messages import (
    CTxOut,
    MAX_BLOCK_WEIGHT,
)
from test_framework.script import (
    CScript,
    OP_CHECKSIG,
    OP_RETURN,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal


class PolicyLimitsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def run_test(self):
        node = self.nodes[0]

        self.log.info("Check default mempool size limit")
        assert_equal(node.getmempoolinfo()["maxmempool"], 500_000_000)

        best_hash = int(node.getbestblockhash(), 16)
        height = node.getblockcount() + 1

        self.log.info("Accept block at MAX_BLOCK_WEIGHT")
        payload = 4_999_837
        script = CScript([OP_RETURN, b"a" * payload])
        coinbase = create_coinbase(height, extra_output_script=script)
        block = create_block(best_hash, coinbase, tmpl={"height": height})
        block.solve()
        assert_equal(block.get_weight(), MAX_BLOCK_WEIGHT)
        assert_equal(node.submitblock(block.serialize().hex()), None)

        self.log.info("Reject block over weight limit")
        best_hash = int(node.getbestblockhash(), 16)
        height = node.getblockcount() + 1
        payload = 4_999_838
        script = CScript([OP_RETURN, b"a" * payload])
        coinbase = create_coinbase(height, extra_output_script=script)
        block = create_block(best_hash, coinbase, tmpl={"height": height})
        block.solve()
        assert_equal(block.get_weight(), MAX_BLOCK_WEIGHT + 4)
        assert node.submitblock(block.serialize().hex()) is not None

        self.log.info("Accept block with 100k sigops")
        best_hash = int(node.getbestblockhash(), 16)
        height = node.getblockcount() + 1
        coinbase = create_coinbase(height)
        sig_script = CScript([OP_CHECKSIG] * 10_000)
        for _ in range(10):
            coinbase.vout.append(CTxOut(0, sig_script))
        block = create_block(best_hash, coinbase, tmpl={"height": height})
        block.solve()
        assert_equal(get_legacy_sigopcount_block(block), MAX_BLOCK_SIGOPS_WEIGHT)
        assert_equal(node.submitblock(block.serialize().hex()), None)

        self.log.info("Reject block over 100k sigops")
        best_hash = int(node.getbestblockhash(), 16)
        height = node.getblockcount() + 1
        coinbase = create_coinbase(height)
        for _ in range(10):
            coinbase.vout.append(CTxOut(0, sig_script))
        coinbase.vout.append(CTxOut(0, CScript([OP_CHECKSIG])))
        block = create_block(best_hash, coinbase, tmpl={"height": height})
        block.solve()
        assert get_legacy_sigopcount_block(block) > MAX_BLOCK_SIGOPS_WEIGHT
        assert node.submitblock(block.serialize().hex()) is not None

        self.log.info("Verify blocksonly mempool limit")
        self.restart_node(0, ["-blocksonly"])
        assert_equal(self.nodes[0].getmempoolinfo()["maxmempool"], 8_000_000)


if __name__ == '__main__':
    PolicyLimitsTest(__file__).main()
