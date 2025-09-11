#!/usr/bin/env python3
"""Verify bech32 HRPs on testnet and signet."""
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

class Bech32HRPTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def setup_network(self):
        self.add_nodes(self.num_nodes)
        # Configure node0 for testnet
        self.nodes[0].chain = 'testnet4'
        self.nodes[0].replace_in_config([('regtest=', 'testnet4='), ('[regtest]', '[testnet4]')])
        self.nodes[0].extra_args = ['-maxconnections=0']
        # Configure node1 for signet
        self.nodes[1].chain = 'signet'
        self.nodes[1].replace_in_config([('regtest=', 'signet='), ('[regtest]', '[signet]')])
        self.nodes[1].extra_args = ['-maxconnections=0']
        self.start_nodes()

    def run_test(self):
        self.log.info("Check Bech32 HRPs for testnet and signet")
        tb_addr = self.nodes[0].getnewaddress(address_type='bech32')
        assert_equal(tb_addr[:2], 'tb')
        sb_addr = self.nodes[1].getnewaddress(address_type='bech32')
        assert_equal(sb_addr[:2], 'sb')

if __name__ == '__main__':
    Bech32HRPTest(__file__).main()
