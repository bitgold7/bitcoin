#!/usr/bin/env python3
"""Verify getstakinginfo reflects staking state and related wallet controls."""

from decimal import Decimal

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal


class WalletStakingInfoTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        node = self.nodes[0]
        addr = node.getnewaddress()
        node.generatetoaddress(200, addr)

        passphrase = "testpass"
        node.encryptwallet(passphrase)
        self.restart_node(0)

        node.walletpassphrase(passphrase, 0, True)

        assert node.walletstaking(True)
        self.wait_until(lambda: node.getstakinginfo()["staking"])
        info = node.getstakinginfo()
        assert info["staking"]
        weight = info["weight"]
        assert weight > 0

        balance = node.getbalance()
        node.reservebalance(True, balance)
        self.wait_until(lambda: node.getstakinginfo()["weight"] == 0)
        assert_equal(node.getstakinginfo()["weight"], 0)

        node.reservebalance(False)
        self.wait_until(lambda: node.getstakinginfo()["weight"] > 0)

        assert_equal(node.setstakesplitthreshold(Decimal("100"))["threshold"], Decimal("100"))
        assert_equal(node.setstakesplitthreshold()["threshold"], Decimal("100"))


if __name__ == "__main__":
    WalletStakingInfoTest(__file__).main()
