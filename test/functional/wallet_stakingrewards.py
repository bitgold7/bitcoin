#!/usr/bin/env python3
from decimal import Decimal
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

class WalletStakingRewardsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [[]]

    def run_test(self):
        stats = self.nodes[0].getstakingstats()
        assert 'staked' in stats
        assert 'current_reward' in stats
        assert 'next_reward_time' in stats
        assert_equal(stats.get('attempts', 0), 0)
        assert_equal(stats.get('successes', 0), 0)
        rewards = self.nodes[0].getstakingrewards()
        assert_equal(rewards, Decimal('0.00000000'))

if __name__ == '__main__':
    WalletStakingRewardsTest().main()
