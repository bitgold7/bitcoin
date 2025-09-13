#!/usr/bin/env python3
"""Test Bulletproof confidential transactions preserve supply and hide amounts."""
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

class BulletproofSupplyTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [[]]

    def run_test(self):
        node = self.nodes[0]
        node.generate(101)
        utxo = node.listunspent()[0]
        addr = node.getnewaddress()
        bp = node.createrawbulletprooftransaction([{"txid": utxo["txid"], "vout": utxo["vout"]}], {addr: utxo["amount"]})
        signed = node.signrawtransactionwithwallet(bp["hex"])['hex']
        dec = node.decoderawtransaction(signed)
        assert_equal(dec['vout'][0]['value'], 0)
        txid = node.sendrawtransaction(signed)
        node.generate(1)
        outs = node.listunspent()
        assert any(o['txid'] == txid for o in outs)

if __name__ == '__main__':
    BulletproofSupplyTest(__file__).main()
