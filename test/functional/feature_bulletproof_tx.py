#!/usr/bin/env python3
"""Create and confirm a Bulletproof confidential transaction."""
from test_framework.test_framework import BitcoinTestFramework

class BulletproofTxTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def run_test(self):
        node = self.nodes[0]
        node.generate(101)
        assert node.getblockchaininfo()["softforks"]["bulletproof"]["active"]
        utxo = node.listunspent()[0]
        addr = node.getnewaddress()
        bp = node.createrawbulletprooftransaction([{"txid": utxo["txid"], "vout": utxo["vout"]}], {addr: utxo["amount"]})
        signed = node.signrawtransactionwithwallet(bp["hex"])["hex"]
        txid = node.sendrawtransaction(signed)
        node.generate(1)
        assert any(o["txid"] == txid for o in node.listunspent())

if __name__ == '__main__':
    BulletproofTxTest(__file__).main()
