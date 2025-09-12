#!/usr/bin/env python3
"""Test mempool priority influences block selection and DoS limits."""

from decimal import Decimal

from test_framework.test_framework import BitcoinTestFramework
from test_framework.wallet import MiniWallet


class MempoolPriorityTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        # Use a small block weight so only one of our large txs fits
        self.extra_args = [["-blockmaxweight=500000"]]

    def run_test(self):
        node = self.nodes[0]
        wallet = MiniWallet(node)

        # Mine enough blocks so that some utxos have more than 7 days age
        self.generate(node, 1300)
        wallet.rescan_utxos()

        # Allow large priority values for comparative checks
        node.setprioritypolicy(True, 200)

        # Select an old utxo and create a freshly confirmed one
        old_utxo = min(wallet.get_utxos(mark_as_spent=False, confirmed_only=True), key=lambda u: u["height"])
        old_utxo = wallet.get_utxo(txid=old_utxo["txid"], vout=old_utxo["vout"])
        fresh_tx = wallet.send_self_transfer(from_node=node)
        self.generate(node, 1)
        wallet.rescan_utxos()
        fresh_utxo = wallet.get_utxo(txid=fresh_tx["txid"], vout=fresh_tx["new_utxo"]["vout"])

        # Craft two large transactions with different fee rates and stake duration
        low_fee_old = wallet.create_self_transfer(
            utxo_to_spend=old_utxo,
            fee_rate=Decimal("0.00001"),  # 1 sat/vB
            target_vsize=100000,
        )
        low_fee_old_id = wallet.sendrawtransaction(from_node=node, tx_hex=low_fee_old["hex"])

        high_fee_new = wallet.create_self_transfer(
            utxo_to_spend=fresh_utxo,
            fee_rate=Decimal("0.001"),  # 100 sat/vB
            target_vsize=100000,
        )
        high_fee_new_id = wallet.sendrawtransaction(from_node=node, tx_hex=high_fee_new["hex"])

        info_old = node.getmempoolentry(low_fee_old_id)
        info_new = node.getmempoolentry(high_fee_new_id)
        old_feerate = info_old["fees"]["base"] / info_old["vsize"]
        new_feerate = info_new["fees"]["base"] / info_new["vsize"]
        assert old_feerate < new_feerate
        assert info_old["priority"] > info_new["priority"]

        block_hash = self.generate(node, 1)[0]
        txids_in_block = node.getblock(block_hash)["tx"][1:]
        assert low_fee_old_id in txids_in_block
        assert high_fee_new_id not in txids_in_block
        assert high_fee_new_id in node.getrawmempool()

        # Clear mempool before next section
        self.generate(node, 1)
        wallet.rescan_utxos()

        # RBF vs non-RBF priority comparison
        node.setprioritypolicy(True, 200)
        non_rbf = wallet.send_self_transfer(from_node=node, sequence=0xFFFFFFFF)
        rbf = wallet.send_self_transfer(from_node=node, sequence=0)
        prio_non_rbf = node.getmempoolentry(non_rbf["txid"])["priority"]
        prio_rbf = node.getmempoolentry(rbf["txid"])["priority"]
        assert prio_rbf < prio_non_rbf

        # Clear mempool
        self.generate(node, 1)
        wallet.rescan_utxos()

        # CPFP boosts child priority
        node.setprioritypolicy(True, 200)
        parent = wallet.send_self_transfer(from_node=node, fee_rate=Decimal("0.00001"))
        child = wallet.create_self_transfer(
            utxo_to_spend=parent["new_utxo"],
            fee_rate=Decimal("0.001"),
        )
        wallet.sendrawtransaction(from_node=node, tx_hex=child["hex"])
        prio_parent = node.getmempoolentry(parent["txid"])["priority"]
        prio_child = node.getmempoolentry(child["txid"])["priority"]
        assert prio_child > prio_parent

        # DoS limit clamps excessive priority values
        node.setprioritypolicy(True, 30)
        spam = wallet.send_self_transfer(from_node=node, fee_rate=Decimal("0.001"))
        assert node.getmempoolentry(spam["txid"])["priority"] == 30


if __name__ == '__main__':
    MempoolPriorityTest(__file__).main()
