#!/usr/bin/env python3
"""Test handling of network magic bytes and the default P2P port."""

from test_framework.test_framework import BitcoinTestFramework, SkipTest
from test_framework.messages import MAGIC_BYTES
from test_framework.p2p import P2PInterface


class P2PMagicBytesTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.chain = ""  # main network
        self.extra_args = [["-port=8888"]]

    def run_test(self):
        if self.options.v2transport:
            raise SkipTest("magic bytes are v1-specific")

        # Override mainnet magic bytes for the BitGold network
        MAGIC_BYTES[""] = b"\xfb\xc0\xc5\xdb"

        self.log.info("Connecting with correct magic to default port 8888")
        good_conn = self.nodes[0].add_p2p_connection(P2PInterface(), dstport=8888)
        good_conn.wait_for_verack()
        self.nodes[0].disconnect_p2ps()

        self.log.info("Connecting with incorrect magic disconnects")
        bad_conn = P2PInterface()
        bad_conn.peer_connect(
            dstaddr="127.0.0.1",
            dstport=8888,
            net="regtest",
            timeout_factor=self.nodes[0].timeout_factor,
            supports_v2_p2p=False,
            send_version=True,
        )()
        bad_conn.wait_for_connect()
        bad_conn.wait_for_disconnect()


if __name__ == "__main__":
    P2PMagicBytesTest(__file__).main()

