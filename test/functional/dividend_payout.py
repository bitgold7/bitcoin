#!/usr/bin/env python3
"""Exercise dividend payout and claiming."""
from decimal import Decimal
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

QUARTER_BLOCKS = 16200

class DividendPayoutTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True
        self.extra_args = [["-dividendpayouts=1"], ["-dividendpayouts=1"]]

    def run_test(self):
        node0, node1 = self.nodes
        addr0 = node0.getnewaddress()
        addr1 = node1.getnewaddress()

        node0.generatetoaddress(1, addr0)
        node0.sendtoaddress(addr1, 1)
        node0.generatetoaddress(1, addr0)

        node0.startstaking()
        node1.startstaking()

        remaining = QUARTER_BLOCKS - node0.getblockcount()
        node0.generatetoaddress(remaining, addr0)

        blockhash = node0.getblockhash(QUARTER_BLOCKS)
        block = node0.getblock(blockhash, 2)
        reward_tx = block["tx"][1]
        assert_equal(reward_tx["vout"][2]["value"], block["tx"][1]["vout"][2]["value"])

        claim0 = node0.claimdividends(addr0)
        claim1 = node1.claimdividends(addr1)
        wallet_claims = node0.claimwalletdividends()

        assert "claimed" in claim0
        assert "claimed" in claim1
        assert isinstance(wallet_claims, dict)

if __name__ == '__main__':
    DividendPayoutTest().main()
