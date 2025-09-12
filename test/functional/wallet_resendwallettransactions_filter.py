#!/usr/bin/env python3
"""Verify wallet rebroadcast respects feerate threshold."""
from decimal import Decimal

from test_framework.test_framework import BitcoinTestFramework
from test_framework.blocktools import create_block, create_coinbase
from test_framework.p2p import P2PTxInvStore
from test_framework.util import assert_equal

class WalletResendThresholdTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        # Require feerate >= 1.5 sat/vB for rebroadcast
        self.extra_args = [["-walletrebroadcastfeerate=0.00001500"]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        node = self.nodes[0]
        peer_initial = node.add_p2p_connection(P2PTxInvStore())

        utxos = node.listunspent()
        addr_high = node.getnewaddress()
        txid_high = node.send(outputs=[{addr_high: 1}], inputs=[utxos[0]], fee_rate=Decimal("0.00002000"))["txid"]
        addr_low = node.getnewaddress()
        txid_low = node.send(outputs=[{addr_low: 1}], inputs=[utxos[1]], fee_rate=Decimal("0.00001000"))["txid"]
        peer_initial.wait_for_broadcast([txid_high, txid_low])

        peer_rebroadcast = node.add_p2p_connection(P2PTxInvStore())

        block_time = node.getblockheader(node.getbestblockhash())["time"] + 6 * 60
        node.setmocktime(block_time)
        block = create_block(int(node.getbestblockhash(), 16), create_coinbase(node.getblockcount() + 1), block_time)
        block.solve()
        node.submitblock(block.serialize().hex())
        node.syncwithvalidationinterfacequeue()

        node.setmocktime(block_time + 36 * 60 * 60)
        node.mockscheduler(60)
        node.setmocktime(block_time + 36 * 60 * 60 + 600)

        assert_equal(txid_high in peer_rebroadcast.get_invs(), True)
        assert_equal(txid_low in peer_rebroadcast.get_invs(), False)

if __name__ == '__main__':
    WalletResendThresholdTest(__file__).main()
