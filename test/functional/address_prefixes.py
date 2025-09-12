#!/usr/bin/env python3
"""Test address and WIF prefixes on main network."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

ALPHABET = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz"

def b58decode(s: str) -> bytes:
    """Decode a base58-encoded string (without checksum verification)."""
    n = 0
    for c in s:
        n = n * 58 + ALPHABET.index(c)
    full = n.to_bytes((n.bit_length() + 7) // 8, 'big') if n > 0 else b""
    pad = 0
    for c in s:
        if c == '1':
            pad += 1
        else:
            break
    return b"\x00" * pad + full

class AddressPrefixesTest(BitcoinTestFramework):
    def set_test_params(self):
        self.chain = ''  # run on main network
        self.num_nodes = 1
        self.setup_clean_chain = True

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        node = self.nodes[0]
        # Legacy P2PKH address
        legacy_addr = node.getnewaddress(address_type='legacy')
        assert legacy_addr[0] == 'B'
        # P2SH address
        script_addr = node.getnewaddress(address_type='p2sh-segwit')
        assert script_addr[0] == 'G'

        # Bech32 address
        bech32_addr = node.getnewaddress("", "bech32")
        assert bech32_addr.startswith('bg1')

        # Dump private key and verify secret key prefix
        wif = node.dumpprivkey(legacy_addr)
        decoded = b58decode(wif)
        assert_equal(decoded[0], 153)  # base58Prefixes[SECRET_KEY]

if __name__ == '__main__':
    AddressPrefixesTest(__file__).main()
