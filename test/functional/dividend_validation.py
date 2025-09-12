#!/usr/bin/env python3
# Copyright (c) 2024 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Ensure blocks enforce dividend fee rules and payouts."""

from decimal import Decimal

from test_framework.test_framework import BitcoinTestFramework
from test_framework.blocktools import create_block, create_coinbase
from test_framework.p2p import P2PInterface
from test_framework.messages import CTxOut
from test_framework.script import CScript, OP_TRUE
from test_framework.util import assert_equal

COIN = 100_000_000
QUARTER_BLOCKS = 16_200
YEAR_BLOCKS = QUARTER_BLOCKS * 4


def btc_to_sat(value):
    return int(Decimal(value) * COIN)


def calculate_apr(amount, blocks_held):
    age_factor = min(1.0, blocks_held / YEAR_BLOCKS)
    amount_factor = min(1.0, amount / (1000 * COIN))
    factor = max(age_factor, amount_factor)
    return 0.01 + 0.09 * factor


def calculate_payouts(stakes, height, pool):
    if pool <= 0 or height % QUARTER_BLOCKS != 0:
        return {}
    pending = []
    total_desired = 0
    for addr, info in stakes.items():
        duration = height - info["last_payout"]
        weight = info["weight"]
        if duration <= 0 or weight <= 0:
            continue
        apr = calculate_apr(weight, duration)
        desired = int(weight * apr / 4.0)
        if desired <= 0:
            continue
        pending.append((addr, desired))
        total_desired += desired
    if total_desired <= 0:
        return {}
    scale = min(1.0, pool / total_desired)
    payouts = {}
    for addr, desired in pending:
        payout = int(desired * scale)
        if payout > 0:
            payouts[addr] = payout
    return payouts


class DividendValidationTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.extra_args = [["-dividendpayouts"]]

    def run_test(self):
        node = self.nodes[0]
        node.p2p = P2PInterface()
        node.p2p.connect_to_node(node)

        tip = int(node.getbestblockhash(), 16)
        height = node.getblockcount() + 1

        # Block missing dividend output
        coinbase = create_coinbase(height)
        block = create_block(tip, coinbase)
        block.solve()
        node.p2p.send_blocks_and_test([block], node, success=False, reject_reason=b"bad-dividend-missing")

        # Block with incorrect dividend amount
        coinbase = create_coinbase(height)
        coinbase.vout.append(CTxOut(0, CScript([OP_TRUE])))
        block = create_block(tip, coinbase)
        block.solve()
        node.p2p.send_blocks_and_test([block], node, success=False, reject_reason=b"bad-dividend-amount")

        # Mine a valid block and verify 90/10 reward split
        addr = node.getnewaddress()
        node.generatetoaddress(1, addr)
        block = node.getblock(node.getbestblockhash(), 2)
        reward_tx = block["tx"][0]
        vout = reward_tx["vout"]
        staker = btc_to_sat(vout[1]["value"])
        pool = btc_to_sat(vout[2]["value"])
        total = staker + pool
        assert_equal(pool, total // 10)
        assert_equal(staker, total - pool)

        # Advance to dividend::QUARTER_BLOCKS and check payout outputs
        node.generatetoaddress(QUARTER_BLOCKS - 2, addr)
        info = node.getdividendinfo()
        stakes = {
            a: {"weight": btc_to_sat(s["weight"]), "last_payout": s["last_payout"]}
            for a, s in info["stakes"].items()
        }
        pool_before = btc_to_sat(info["pool"])
        node.generatetoaddress(1, addr)
        block = node.getblock(node.getbestblockhash(), 2)
        reward_tx = block["tx"][0]
        payouts = calculate_payouts(stakes, QUARTER_BLOCKS, pool_before)
        vouts = reward_tx["vout"][3:]
        assert_equal(len(vouts), len(payouts))
        for txout, (addr, amount) in zip(vouts, sorted(payouts.items())):
            assert_equal(btc_to_sat(txout["value"]), amount)


if __name__ == "__main__":
    DividendValidationTest(__file__).main()

