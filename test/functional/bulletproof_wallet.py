#!/usr/bin/env python3
"""Test Bulletproof wallet interactions across nodes."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal


class BulletproofWalletTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 3
        self.extra_args = [[], [], []]

    def run_test(self):
        node_a, node_b, node_c = self.nodes
        node_a.generate(101)
        utxo = node_a.listunspent()[0]
        addr_b = node_b.getnewaddress()
        bp = node_a.createrawbulletprooftransaction(
            [{"txid": utxo["txid"], "vout": utxo["vout"]}],
            {addr_b: utxo["amount"]},
        )
        signed = node_a.signrawtransactionwithwallet(bp["hex"])["hex"]

        dec_c = node_c.decoderawtransaction(signed)
        assert_equal(dec_c["vout"][0]["value"], 0)

        txid = node_a.sendrawtransaction(signed)
        node_a.generate(1)

        assert_equal(node_b.getbalance(), 0)
        node_b.rescanblockchain()
        unspents = node_b.listunspent()
        found = [o for o in unspents if o["txid"] == txid]
        assert_equal(len(found), 1)
        assert_equal(found[0]["amount"], utxo["amount"])

        addr_a = node_a.getnewaddress()
        txid2 = node_b.sendtoaddress(addr_a, utxo["amount"])
        node_a.generate(1)
        outs = node_a.listunspent()
        assert any(o["txid"] == txid2 for o in outs)


if __name__ == "__main__":
    BulletproofWalletTest(__file__).main()
