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

        # Tampered commitment -> bad-bulletproof
        tx3 = tx_from_hex(created["hex"])
        elems3 = list(CScript(tx3.vout[0].scriptPubKey))
        corrupt = bytearray(elems3[1])
        corrupt[0] ^= 1
        tx3.vout[0].scriptPubKey = CScript([CScriptOp(0xbb), bytes(corrupt), elems3[2]])
        tx3.rehash()
        res3 = node.testmempoolaccept([tx3.serialize().hex()])[0]
        assert_equal(res3["reject-reason"], "bad-bulletproof")

        # Balance violation -> bad-bulletproof-balance
        addr1 = node.getnewaddress()
        addr2 = node.getnewaddress()
        created2 = node.createrawbulletprooftransaction([], {addr1: 0, addr2: 0})
        tx4 = tx_from_hex(created2["hex"])
        tx4.vout[1].scriptPubKey = CScript([])
        tx4.rehash()
        res4 = node.testmempoolaccept([tx4.serialize().hex()])[0]
        assert_equal(res4["reject-reason"], "bad-bulletproof-balance")


if __name__ == '__main__':
    BulletproofInvalidTest(__file__).main()
