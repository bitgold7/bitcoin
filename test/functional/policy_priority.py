#!/usr/bin/env python3
"""Tests for priority-aware mempool selection and mining policy."""

from decimal import Decimal

from test_framework.messages import COIN
from test_framework.test_framework import BitcoinTestFramework
from test_framework.wallet import MiniWallet


class PrioritySelectionPolicyTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.extra_args = [["-blockmintxfee=0"]]

    def run_test(self):
        self.wallet = MiniWallet(self.nodes[0])
        self._test_stake_priority()

        # Restart to clear the mempool and reuse the matured coins for duration tests
        self.restart_node(0, extra_args=["-blockmintxfee=0"])
        self.wallet = MiniWallet(self.nodes[0])
        self._test_duration_priority()

        # Restart with a constrained mempool to exercise congestion behaviour
        self.restart_node(0, extra_args=["-blockmintxfee=0", "-maxmempool=1"])
        self.wallet = MiniWallet(self.nodes[0])
        self._test_congestion_penalty()

    def _template_tx_order(self):
        template = self.nodes[0].getblocktemplate()
        return [entry["txid"] for entry in template["transactions"]]

    def _test_stake_priority(self):
        node = self.nodes[0]
        wallet = self.wallet

        self.log.info("Stake amount should dominate over higher fee when priorities differ")
        self.generate(node, 150)
        wallet.rescan_utxos()

        # Build a large 500-coin UTXO.
        large_inputs = [wallet.get_utxo(confirmed_only=True) for _ in range(10)]
        combine_tx = wallet.send_self_transfer_multi(
            from_node=node,
            utxos_to_spend=large_inputs,
            num_outputs=1,
            fee_per_output=1000,
        )
        self.generate(node, 1)
        wallet.rescan_utxos()

        # Create a small 10-coin UTXO from a different source.
        available = wallet.get_utxos(mark_as_spent=False, confirmed_only=True)
        small_source_info = next(u for u in available if u["txid"] != combine_tx["txid"])
        small_source = wallet.get_utxo(txid=small_source_info["txid"], vout=small_source_info["vout"], confirmed_only=True)
        small_tx = wallet.send_self_transfer_multi(
            from_node=node,
            utxos_to_spend=[small_source],
            num_outputs=1,
            amount_per_output=int(10 * COIN),
            fee_per_output=1000,
        )
        self.generate(node, 1)
        wallet.rescan_utxos()

        high_utxo = wallet.get_utxo(txid=combine_tx["txid"], vout=0, confirmed_only=True)
        low_utxo = wallet.get_utxo(txid=small_tx["txid"], vout=0, confirmed_only=True)

        high_tx = wallet.send_self_transfer(from_node=node, utxo_to_spend=high_utxo, fee_rate=Decimal("0.00005"))
        low_tx = wallet.send_self_transfer(from_node=node, utxo_to_spend=low_utxo, fee_rate=Decimal("0.005"))

        order = self._template_tx_order()
        assert order.index(high_tx["txid"]) < order.index(low_tx["txid"])

        # Clear the mempool before the next scenario.
        self.generate(node, 1)
        wallet.rescan_utxos()

    def _test_duration_priority(self):
        node = self.nodes[0]
        wallet = self.wallet

        self.log.info("Older stakes should be prioritised over fresh stakes with equal fees")
        wallet.rescan_utxos()

        # Take an existing matured UTXO and age it via additional blocks.
        old_candidate = wallet.get_utxo(confirmed_only=True)
        self.generate(node, 1300)
        wallet.rescan_utxos()

        # Create a fresh 50-coin UTXO near the tip.
        available = wallet.get_utxos(mark_as_spent=False, confirmed_only=True)
        recent_source_info = next(u for u in available if not (u["txid"] == old_candidate["txid"] and u["vout"] == old_candidate["vout"]))
        recent_source = wallet.get_utxo(txid=recent_source_info["txid"], vout=recent_source_info["vout"], confirmed_only=True)
        fresh_tx = wallet.send_self_transfer_multi(
            from_node=node,
            utxos_to_spend=[recent_source],
            num_outputs=1,
            amount_per_output=int(50 * COIN),
            fee_per_output=1000,
        )
        self.generate(node, 1)
        wallet.rescan_utxos()

        old_utxo = wallet.get_utxo(txid=old_candidate["txid"], vout=old_candidate["vout"], confirmed_only=True)
        fresh_utxo = wallet.get_utxo(txid=fresh_tx["txid"], vout=0, confirmed_only=True)

        old_tx = wallet.send_self_transfer(from_node=node, utxo_to_spend=old_utxo, fee_rate=Decimal("0.0002"))
        new_tx = wallet.send_self_transfer(from_node=node, utxo_to_spend=fresh_utxo, fee_rate=Decimal("0.0002"))

        order = self._template_tx_order()
        assert order.index(old_tx["txid"]) < order.index(new_tx["txid"])

        self.generate(node, 1)
        wallet.rescan_utxos()

    def _test_congestion_penalty(self):
        node = self.nodes[0]
        wallet = self.wallet

        self.log.info("Congestion penalties should demote transactions accepted late")
        self.generate(node, 800)
        wallet.rescan_utxos()

        info = node.getmempoolinfo()
        max_bytes = info["maxmempool"] * 1_000_000
        threshold = int(max_bytes * 0.9)
        buffer = 200_000

        # Reserve UTXOs for the priority comparison before filling the mempool.
        pre_utxo = wallet.get_utxo(confirmed_only=True)
        post_utxo = wallet.get_utxo(confirmed_only=True)

        def add_filler():
            filler_utxo = wallet.get_utxo(confirmed_only=True)
            wallet.send_self_transfer_multi(
                from_node=node,
                utxos_to_spend=[filler_utxo],
                num_outputs=120,
                fee_per_output=1000,
            )

        # Raise mempool usage close to, but below, the congestion threshold.
        for _ in range(400):
            if node.getmempoolinfo()["usage"] >= threshold - buffer:
                break
            add_filler()
        else:
            raise AssertionError("Failed to reach target pre-congestion usage")

        assert node.getmempoolinfo()["usage"] < threshold
        tx_pre = wallet.send_self_transfer(from_node=node, utxo_to_spend=pre_utxo, fee_rate=Decimal("0.0005"))

        # Push the mempool above the congestion threshold.
        for _ in range(400):
            if node.getmempoolinfo()["usage"] > threshold:
                break
            add_filler()
        else:
            raise AssertionError("Failed to exceed congestion threshold")

        assert node.getmempoolinfo()["usage"] > threshold
        tx_post = wallet.send_self_transfer(from_node=node, utxo_to_spend=post_utxo, fee_rate=Decimal("0.0005"))

        order = self._template_tx_order()
        assert order.index(tx_pre["txid"]) < order.index(tx_post["txid"])


if __name__ == '__main__':
    PrioritySelectionPolicyTest(__file__).main()
