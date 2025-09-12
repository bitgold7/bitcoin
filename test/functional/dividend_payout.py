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

        # Send a mempool transaction and ensure it is included in the staked block
        txid = node0.sendtoaddress(addr1, 1)
        start_height = node0.getblockcount()
        node0.waitforblockheight(start_height + 1)
        blockhash = node0.getblockhash(start_height + 1)
        block = node0.getblock(blockhash, 2)
        assert txid in [tx["txid"] for tx in block["tx"][2:]]

        coinstake = block["tx"][1]
        input_total = Decimal("0")
        for vin in coinstake["vin"]:
            prev = node0.getrawtransaction(vin["txid"], True)
            input_total += prev["vout"][vin["vout"]]["value"]
        validator_value = sum(vout["value"] for vout in coinstake["vout"][1:-1])
        dividend_value = coinstake["vout"][-1]["value"]
        validator_reward = validator_value - input_total
        assert_equal(validator_reward, dividend_value * 9)

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
