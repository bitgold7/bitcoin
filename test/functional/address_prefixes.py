#!/usr/bin/env python3
"""Test address prefixes on main, test, signet, and reg networks."""

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
        self.num_nodes = 4
        self.setup_clean_chain = True

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def setup_network(self):
        self.add_nodes(self.num_nodes)
        # Configure node0 for main network
        self.nodes[0].chain = ''
        self.nodes[0].replace_in_config([('regtest=1\n', ''), ('[regtest]\n', '')])
        self.nodes[0].extra_args = ['-maxconnections=0']
        # Configure node1 for testnet
        self.nodes[1].chain = 'testnet4'
        self.nodes[1].replace_in_config([('regtest=1\n', 'testnet4=1\n'), ('[regtest]\n', '[testnet4]\n')])
        self.nodes[1].extra_args = ['-maxconnections=0']
        # Configure node2 for signet
        self.nodes[2].chain = 'signet'
        self.nodes[2].replace_in_config([('regtest=1\n', 'signet=1\n'), ('[regtest]\n', '[signet]\n')])
        self.nodes[2].extra_args = ['-maxconnections=0']
        # Configure node3 for regtest (default)
        self.nodes[3].extra_args = ['-maxconnections=0']
        self.start_nodes()

    def run_test(self):
        # Main network checks
        main = self.nodes[0]
        legacy = main.getnewaddress(address_type='legacy')
        assert legacy[0] == 'B'
        script = main.getnewaddress(address_type='p2sh-segwit')
        assert script[0] == 'G'
        bech32 = main.getnewaddress("", "bech32")
        assert bech32.startswith('bg1')
        wif = main.dumpprivkey(legacy)
        decoded = b58decode(wif)
        assert_equal(decoded[0], 153)

        # Testnet checks
        test = self.nodes[1]
        t_legacy = test.getnewaddress(address_type='legacy')
        assert_equal(b58decode(t_legacy)[0], 65)
        t_script = test.getnewaddress(address_type='p2sh-segwit')
        assert_equal(b58decode(t_script)[0], 78)
        t_bech32 = test.getnewaddress("", "bech32")
        assert t_bech32.startswith('tbg1')

        # Signet checks
        sig = self.nodes[2]
        s_legacy = sig.getnewaddress(address_type='legacy')
        assert_equal(b58decode(s_legacy)[0], 111)
        s_script = sig.getnewaddress(address_type='p2sh-segwit')
        assert_equal(b58decode(s_script)[0], 196)
        s_bech32 = sig.getnewaddress("", "bech32")
        assert s_bech32.startswith('sbg1')

        # Regtest checks
        reg = self.nodes[3]
        r_legacy = reg.getnewaddress(address_type='legacy')
        assert_equal(b58decode(r_legacy)[0], 111)
        r_script = reg.getnewaddress(address_type='p2sh-segwit')
        assert_equal(b58decode(r_script)[0], 196)
        r_bech32 = reg.getnewaddress("", "bech32")
        assert r_bech32.startswith('rbg1')

if __name__ == '__main__':
    AddressPrefixesTest(__file__).main()
