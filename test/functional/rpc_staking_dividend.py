#!/usr/bin/env python3
"""Test staking and dividend RPCs."""
from decimal import Decimal
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error

class StakingDividendRPCTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def run_test(self):
        node = self.nodes[0]

        # priority engine toggling
        assert node.priorityengine()
        assert_equal(node.priorityengine(False), False)
        assert_equal(node.priorityengine(), False)
        assert_equal(node.priorityengine(True), True)
        assert_raises_rpc_error(-1, "JSON value is not boolean", node.priorityengine, "bad")

        # staking start/stop
        assert node.startstaking()
        assert_equal(node.stopstaking(), False)

        # dividend pool operations
        pool = node.getdividendpool()
        assert "amount" in pool
        assert pool["amount"] > 0
        assert_raises_rpc_error(-8, "address and amount required", node.claimdividends)
        assert_raises_rpc_error(-8, "amount must be positive", node.claimdividends, "addr", Decimal("-1"))
        assert_raises_rpc_error(-8, "insufficient dividend pool", node.claimdividends, "addr", pool["amount"] + 1)
        res = node.claimdividends("addr", Decimal("1"))
        assert_equal(res["claimed"], Decimal("1"))
        new_pool = node.getdividendpool()
        assert_equal(new_pool["amount"], pool["amount"] - Decimal("1"))

        # dividend schedule
        stakes = [
            {"address": "A", "weight": Decimal("10"), "last_payout_height": 0},
            {"address": "B", "weight": Decimal("20"), "last_payout_height": 0},
        ]
        schedule = node.getdividendschedule(stakes, 10, Decimal("30"))
        assert "A" in schedule and "B" in schedule
        assert_raises_rpc_error(-8, "stakes, height, pool required", node.getdividendschedule)

        # expanded dividend RPCs
        info = node.getdividendinfo()
        assert "pool" in info and "stakes" in info
        est = node.estimatedividend("addr")
        assert "amount" in est
        elig = node.getdividendeligibility("addr")
        assert "eligible" in elig

if __name__ == '__main__':
    StakingDividendRPCTest(__file__).main()
