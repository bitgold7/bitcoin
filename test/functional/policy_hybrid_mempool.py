#!/usr/bin/env python3
"""Test hybrid mempool policy ordering."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.wallet import MiniWallet
from test_framework.util import assert_equal
import time


class HybridMempoolPolicyTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [['-hybridmempool']]

    def run_test(self):
        node = self.nodes[0]
        wallet = MiniWallet(node)
        wallet.generate(101)

        utxos = [wallet.get_utxo() for _ in range(7)]

        # Fee ordering: higher fee rate should be mined first
        low_fee = wallet.send_self_transfer_multi(from_node=node, utxos_to_spend=[utxos[0]], target_vsize=200, fee_per_output=100)
        time.sleep(1)
        high_fee = wallet.send_self_transfer_multi(from_node=node, utxos_to_spend=[utxos[1]], target_vsize=200, fee_per_output=200)
        blockhash = wallet.generate(1)[0]
        block = node.getblock(blockhash)
        assert_equal(block['tx'][1:], [high_fee['txid'], low_fee['txid']])

        # Stake weight: with equal fees, more inputs should win
        one_input = wallet.send_self_transfer_multi(from_node=node, utxos_to_spend=[utxos[2]], target_vsize=200, fee_per_output=200)
        time.sleep(1)
        two_inputs = wallet.send_self_transfer_multi(from_node=node, utxos_to_spend=[utxos[3], utxos[4]], target_vsize=300, fee_per_output=300)
        blockhash = wallet.generate(1)[0]
        block = node.getblock(blockhash)
        assert_equal(block['tx'][1:], [two_inputs['txid'], one_input['txid']])

        # Time in mempool: earlier tx wins when other factors equal
        early = wallet.send_self_transfer_multi(from_node=node, utxos_to_spend=[utxos[5]], target_vsize=200, fee_per_output=200)
        time.sleep(1)
        late = wallet.send_self_transfer_multi(from_node=node, utxos_to_spend=[utxos[6]], target_vsize=200, fee_per_output=200)
        blockhash = wallet.generate(1)[0]
        block = node.getblock(blockhash)
        assert_equal(block['tx'][1:], [early['txid'], late['txid']])


if __name__ == '__main__':
    HybridMempoolPolicyTest(__file__).main()

