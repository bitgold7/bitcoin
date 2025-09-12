#!/usr/bin/env python3
"""Verify address prefixes for the BitGold network.

This test checks that different address types use the expected human-readable
prefixes as defined by the chain parameters.
"""

from test_framework.address import base58_to_byte
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal


class AddressPrefixesTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        node = self.nodes[0]

        legacy_addr = node.getnewaddress(address_type="legacy")
        assert legacy_addr.startswith("B")

        script_addr = node.getnewaddress(address_type="p2sh-segwit")
        assert script_addr.startswith("G")

        bech32_addr = node.getnewaddress("", "bech32")
        assert bech32_addr.startswith("bg1")

        wif = node.dumpprivkey(legacy_addr)
        _, version = base58_to_byte(wif)
        assert_equal(version, 153)


if __name__ == '__main__':
    AddressPrefixesTest(__file__).main()
