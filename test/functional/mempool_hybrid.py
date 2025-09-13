#!/usr/bin/env python3
"""Verify hybrid mempool policy and DoS caps."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.wallet import MiniWallet
from test_framework.util import assert_equal
import time

class MempoolHybridTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        node = self.nodes[0]
        wallet = MiniWallet(node)

        # Default policy is feerate: higher fee rate mines first
        wallet.generate(101)
        utxos = [wallet.get_utxo() for _ in range(4)]
        low_fee_high_stake = wallet.send_self_transfer_multi(
            from_node=node,
            utxos_to_spend=[utxos[0], utxos[1]],
            target_vsize=200,
            fee_per_output=100,
        )
        time.sleep(1)
        high_fee_low_stake = wallet.send_self_transfer_multi(
            from_node=node,
            utxos_to_spend=[utxos[2]],
            target_vsize=200,
            fee_per_output=200,
        )
        blockhash = wallet.generate(1)[0]
        block = node.getblock(blockhash)
        assert_equal(block['tx'][1:], [high_fee_low_stake['txid'], low_fee_high_stake['txid']])

        # Restart with hybrid policy
        self.restart_node(0, extra_args=['-txpolicy=hybrid'])
        node = self.nodes[0]
        wallet = MiniWallet(node)
        wallet.generate(101)
        utxos = [wallet.get_utxo() for _ in range(20)]
        low_fee_high_stake = wallet.send_self_transfer_multi(
            from_node=node,
            utxos_to_spend=utxos[:2],
            target_vsize=200,
            fee_per_output=100,
        )
        time.sleep(1)
        high_fee_low_stake = wallet.send_self_transfer_multi(
            from_node=node,
            utxos_to_spend=[utxos[2]],
            target_vsize=200,
            fee_per_output=200,
        )
        blockhash = wallet.generate(1)[0]
        block = node.getblock(blockhash)
        assert_equal(block['tx'][1:], [low_fee_high_stake['txid'], high_fee_low_stake['txid']])

        # DoS caps: stake weight saturates at 255 coins
        # Create two huge-stake transactions with equal feerate
        big1 = wallet.send_self_transfer_multi(
            from_node=node,
            utxos_to_spend=utxos[3:9],
            target_vsize=300,
            fee_per_output=200,
        )
        time.sleep(1)
        big2 = wallet.send_self_transfer_multi(
            from_node=node,
            utxos_to_spend=utxos[9:17],
            target_vsize=300,
            fee_per_output=200,
        )
        blockhash = wallet.generate(1)[0]
        block = node.getblock(blockhash)
        # Earlier transaction should win even though second has more stake
        assert_equal(block['tx'][1:], [big1['txid'], big2['txid']])

if __name__ == '__main__':
    MempoolHybridTest(__file__).main()
