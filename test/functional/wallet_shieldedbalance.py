#!/usr/bin/env python3
"""Test staking, dividend, and shielded wallet RPCs."""
from decimal import Decimal
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error

class WalletShieldedBalanceTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [["-dividendpayouts=1"]]

    def run_test(self):
        node = self.nodes[0]
        addr = node.getnewaddress()
        node.generatetoaddress(101, addr)

        # getstakinginfo success and failure
        info = node.getstakinginfo()
        assert "enabled" in info
        assert_raises_rpc_error(-8, "getstakinginfo", node.getstakinginfo, True)

        # reservebalance failure and success
        assert_raises_rpc_error(-8, "amount required", node.reservebalance, True)
        res = node.reservebalance(True, Decimal("1"))
        assert_equal(res["reserved"], Decimal("1"))
        res = node.reservebalance(False)
        assert_equal(res["reserved"], Decimal("0"))

        # setstakesplitthreshold failure and success
        assert_raises_rpc_error(-8, "Amount out of range", node.setstakesplitthreshold, Decimal("-1"))
        res = node.setstakesplitthreshold(Decimal("2"))
        assert_equal(res["threshold"], Decimal("2"))

        # getdividendinfo success and failure
        div = node.getdividendinfo()
        assert "pool" in div
        assert_raises_rpc_error(-8, "getdividendinfo", node.getdividendinfo, 1)

        # sendshielded failures
        assert_raises_rpc_error(-5, "Invalid address", node.sendshielded, "badaddr", Decimal("1"))
        shield_addr = node.getnewshieldedaddress()
        assert_raises_rpc_error(-8, "Amount must be positive", node.sendshielded, shield_addr, Decimal("-1"))

        # sendshielded success
        node.sendshielded(shield_addr, Decimal("1"))

        # getshieldedbalance success and failure
        assert_equal(node.getshieldedbalance(), Decimal("1"))
        assert_raises_rpc_error(-8, "getshieldedbalance", node.getshieldedbalance, True)

if __name__ == '__main__':
    WalletShieldedBalanceTest(__file__).main()
