#!/usr/bin/env python3
# Copyright (c) 2024 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import BitcoinTestFramework
from test_framework.p2p import P2PInterface
from test_framework.wallet import MiniWallet
from test_framework.messages import MSG_TX, MSG_WTX
from test_framework.util import assert_equal

class BlockRelayNoTxInvTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [[]]

    def run_test(self):
        node = self.nodes[0]
        wallet = MiniWallet(node)
        self.generate(node, 101)

        full_peer = node.add_p2p_connection(P2PInterface())
        block_peer = node.add_outbound_p2p_connection(P2PInterface(), p2p_idx=1, connection_type="block-relay-only")

        # broadcast a transaction
        txid = wallet.send_self_transfer()["txid"]

        # full relay peer should receive the transaction announcement
        full_peer.wait_for_inv([txid])

        # block-relay-only peer should not receive tx invs
        block_peer.sync_with_ping()
        inv = block_peer.last_message.get("inv")
        if inv:
            for item in inv.inv:
                assert item.type not in (MSG_TX, MSG_WTX)
        else:
            assert_equal(block_peer.message_count.get("inv", 0), 0)

if __name__ == '__main__':
    BlockRelayNoTxInvTest(__file__).main()
