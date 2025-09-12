#!/usr/bin/env python3
from decimal import Decimal
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

class WalletStakingRewardsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.extra_args = [[]]

    def run_test(self):
        node = self.nodes[0]
        stats = node.getstakingstats()
        assert 'staked' in stats
        assert 'current_reward' in stats
        assert 'next_reward_time' in stats
        assert_equal(stats.get('attempts', 0), 0)
        assert_equal(stats.get('successes', 0), 0)
        rewards = node.getstakingrewards()
        assert_equal(rewards, Decimal('0.00000000'))

        addr = node.getnewaddress()
        blockhash = node.generatetoaddress(1, addr)[0]
        block = node.getblock(blockhash, 2)
        coinbase = block["tx"][0]
        assert len(coinbase["vout"]) >= 2
        dividend = coinbase["vout"][1]
        total_reward = Decimal(coinbase["vout"][0]["value"]) + Decimal(dividend["value"])
        assert_equal(Decimal(dividend["value"]), total_reward / Decimal(10))
        assert_equal(dividend["scriptPubKey"]["hex"], "51")

if __name__ == '__main__':
    WalletStakingRewardsTest(__file__).main()
