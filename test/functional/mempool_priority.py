#!/usr/bin/env python3
from test_framework.test_framework import BitcoinTestFramework
from test_framework.wallet import MiniWallet

class MempoolPriorityTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [["-maxmempool=0.1"]]

    def run_test(self):
        node = self.nodes[0]
        wallet = MiniWallet(node)
        wallet.rescan_utxos()
        self.generate(node, 101)

        for _ in range(18):
            wallet.send_self_transfer_multi(target_vsize=5000)
        assert node.getmempoolinfo()["usage"] > 90000

        node.setprioritypolicy(True, 30)

        non_rbf = wallet.send_self_transfer(sequence=0xFFFFFFFF)
        rbf = wallet.send_self_transfer(sequence=0xFFFFFFFD)
        prio_non_rbf = node.getmempoolentry(non_rbf["txid"])["priority"]
        prio_rbf = node.getmempoolentry(rbf["txid"])["priority"]
        assert prio_rbf < prio_non_rbf

        parent = wallet.send_self_transfer(fee_rate=1)
        child = wallet.create_self_transfer(utxo_to_spend=parent["new_utxos"][0], fee_rate=2000)
        wallet.sendrawtransaction(from_node=node, tx_hex=child["hex"])
        prio_parent = node.getmempoolentry(parent["txid"])["priority"]
        prio_child = node.getmempoolentry(child["txid"])["priority"]
        assert prio_child > prio_parent
        assert prio_child <= 30

if __name__ == '__main__':
    MempoolPriorityTest(__file__).main()
