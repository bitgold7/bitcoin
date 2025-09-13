#!/usr/bin/env python3
"""Import the genesis private key and verify balance after maturity."""

from decimal import Decimal

from test_framework.test_framework import BitcoinTestFramework
from test_framework.blocktools import COINBASE_MATURITY
from test_framework.util import assert_equal

GENESIS_PRIVKEY = "5HpHagT65TZzG1PH3CSu63k8DbpvD8s5ip4nEB3kEsreAnchuDf"
GENESIS_BALANCE = Decimal("3000000")


class GenesisBalanceTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.chain = ""  # main network

    def run_test(self):
        node = self.nodes[0]

        node.createwallet("genesis")
        node.createwallet("miner")
        wgen = node.get_wallet_rpc("genesis")
        wminer = node.get_wallet_rpc("miner")

        wgen.importprivkey(GENESIS_PRIVKEY)
        assert_equal(wgen.getbalance(), Decimal("0"))

        miner_addr = wminer.getnewaddress()
        node.generatetoaddress(COINBASE_MATURITY, miner_addr)

        assert_equal(wgen.getbalance(), GENESIS_BALANCE)


if __name__ == "__main__":
    GenesisBalanceTest(__file__).main()
