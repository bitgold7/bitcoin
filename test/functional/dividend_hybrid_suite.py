#!/usr/bin/env python3
"""Combined dividend scenarios under hybrid consensus."""

from decimal import Decimal

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

QUARTER_BLOCKS = 16200


class DividendHybridSuite(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.extra_args = [["-dividendpayouts=1"], ["-dividendpayouts=1"]]

    def stake_block_with_tx(self, staker, receiver):
        """Stake a block ensuring a mempool tx is included."""
        staker.startstaking()
        receiver.startstaking()
        txid = staker.sendtoaddress(receiver.getnewaddress(), 1)
        start_height = staker.getblockcount()
        staker.waitforblockheight(start_height + 1)
        blockhash = staker.getblockhash(start_height + 1)
        block = staker.getblock(blockhash, 2)
        assert txid in [tx["txid"] for tx in block["tx"][2:]]
        return block

    def generate_to_payout(self, node, addr):
        remaining = QUARTER_BLOCKS - node.getblockcount()
        node.generatetoaddress(remaining, addr)

    def run_test(self):
        node0, node1 = self.nodes
        addr0 = node0.getnewaddress()
        addr1 = node1.getnewaddress()

        node0.generatetoaddress(1, addr0)
        node0.sendtoaddress(addr1, 1)
        node0.generatetoaddress(1, addr0)

        block = self.stake_block_with_tx(node0, node1)

        coinstake = block["tx"][1]
        input_total = Decimal("0")
        for vin in coinstake["vin"]:
            prev = node0.getrawtransaction(vin["txid"], True)
            input_total += prev["vout"][vin["vout"]]["value"]
        validator_value = sum(vout["value"] for vout in coinstake["vout"][1:-1])
        dividend_value = coinstake["vout"][-1]["value"]
        assert_equal(validator_value - input_total, dividend_value * 9)

        self.generate_to_payout(node0, addr0)

        history = node0.getdividendhistory()
        assert str(QUARTER_BLOCKS) in history
        pool = node0.getdividendpool()
        assert_equal(pool["amount"], Decimal("0"))

        claim0 = node0.claimdividends(addr0)
        claim1 = node1.claimdividends(addr1)
        assert "claimed" in claim0
        assert "claimed" in claim1


if __name__ == "__main__":
    DividendHybridSuite().main()
