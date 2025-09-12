#!/usr/bin/env python3
"""Test network identity checks for foreign peers"""
from test_framework.test_framework import BitcoinTestFramework
from test_framework.p2p import P2PInterface
from test_framework.messages import msg_version
import socket

class NetworkIdentityTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        # Listen on the default regtest port so that port checks succeed
        self.extra_args = [["-port=18444"]]

    def run_test(self):
        node = self.nodes[0]

        self.log.info("Test disconnect on wrong network magic")
        wrong_magic = b"\x0b\x11\x09\x07"  # testnet magic
        header = wrong_magic + b"version\x00\x00\x00\x00\x00" + b"\x00\x00\x00\x00" + b"\x00\x00\x00\x00"
        with node.assert_debug_log(["Header error: Wrong MessageStart"]):
            s = socket.create_connection(("127.0.0.1", node.p2p_port))
            s.sendall(header)
            s.close()

        self.log.info("Test disconnect on wrong port")
        wrong_port = 18333
        with node.assert_debug_log([f"unexpected port {wrong_port}"]):
            peer = P2PInterface()
            node.add_p2p_connection(peer, send_version=False)
            ver = msg_version()
            ver.nVersion = 70015
            ver.addrTo.port = wrong_port
            peer.send_message(ver)
            peer.wait_for_disconnect()

if __name__ == '__main__':
    NetworkIdentityTest(__file__).main()
