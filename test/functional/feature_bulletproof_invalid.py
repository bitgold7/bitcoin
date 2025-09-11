#!/usr/bin/env python3
"""Test that invalid Bulletproof proofs are rejected with proper errors."""

from test_framework.authproxy import JSONRPCException
from test_framework.messages import tx_from_hex
from test_framework.script import CScript, CScriptOp
from test_framework.test_framework import BitcoinTestFramework, SkipTest
from test_framework.util import assert_equal


class BulletproofInvalidTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def run_test(self):
        node = self.nodes[0]
        try:
            created = node.createrawbulletprooftransaction([], {"data": "00"})
        except JSONRPCException:
            raise SkipTest("Bulletproof RPC not available")

        tx = tx_from_hex(created["hex"])
        elems = list(CScript(tx.vout[0].scriptPubKey))
        commitment = elems[1]

        # Empty proof -> bad-bulletproof
        tx.vout[0].scriptPubKey = CScript([CScriptOp(0xbb), commitment, b""])
        tx.rehash()
        res = node.testmempoolaccept([tx.serialize().hex()])[0]
        assert_equal(res["reject-reason"], "bad-bulletproof")

        # Missing Bulletproof data -> bad-bulletproof-missing
        tx2 = tx_from_hex(created["hex"])
        tx2.vout[0].scriptPubKey = CScript([])
        tx2.rehash()
        res2 = node.testmempoolaccept([tx2.serialize().hex()])[0]
        assert_equal(res2["reject-reason"], "bad-bulletproof-missing")


if __name__ == '__main__':
    BulletproofInvalidTest().main()
