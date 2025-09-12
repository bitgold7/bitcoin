#!/usr/bin/env python3
"""Test staking coin maturity."""

from decimal import Decimal

from test_framework.test_framework import BitcoinTestFramework
from test_framework.blocktools import COINBASE_MATURITY
from test_framework.messages import COIN, hash256, uint256_from_compact
from test_framework.util import assert_raises_rpc_error, assert_equal

STAKE_TIMESTAMP_MASK = None
MIN_STAKE_AGE = 60 * 60


def check_kernel(prev_hash, prev_height, prev_time, nbits, stake_hash, stake_time, amount, prevout, ntime):
    if ntime & STAKE_TIMESTAMP_MASK:
        return False
    if ntime <= stake_time or ntime - stake_time < MIN_STAKE_AGE:
        return False
    stake_modifier = hash256(
        bytes.fromhex(prev_hash)[::-1]
        + prev_height.to_bytes(4, "little")
        + prev_time.to_bytes(4, "little")
    )
    ntime_masked = ntime & ~STAKE_TIMESTAMP_MASK
    stake_time_masked = stake_time & ~STAKE_TIMESTAMP_MASK
    data = (
        stake_modifier
        + bytes.fromhex(stake_hash)[::-1]
        + stake_time_masked.to_bytes(4, "little")
        + bytes.fromhex(prevout["txid"])[::-1]
        + prevout["vout"].to_bytes(4, "little")
        + ntime_masked.to_bytes(4, "little")
    )
    proof = hash256(data)
    target = uint256_from_compact(nbits) * (amount // COIN)
    return int.from_bytes(proof[::-1], "big") <= target


class CoinMaturityTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def find_ntime(self, prev_hash, prev_height, prev_time, nbits, stake_hash, stake_time, amount, prevout):
        ntime = prev_time + 16
        while not check_kernel(
            prev_hash,
            prev_height,
            prev_time,
            nbits,
            stake_hash,
            stake_time,
            amount,
            prevout,
            ntime,
        ):
            ntime += 16
        return ntime

    def run_test(self):
        node = self.nodes[0]
        global STAKE_TIMESTAMP_MASK
        STAKE_TIMESTAMP_MASK = node.getblockchaininfo()["pos_timestamp_mask"]

        addr = node.getnewaddress()
        node.generatetoaddress(COINBASE_MATURITY + 1, addr)

        # Select a mature UTXO to stake
        unspent = next(u for u in node.listunspent() if u["confirmations"] >= COINBASE_MATURITY)
        amount = int(unspent["amount"] * COIN)
        prevout = {"txid": unspent["txid"], "vout": unspent["vout"]}

        prev_height = node.getblockcount()
        prev_hash = node.getbestblockhash()
        prev_block = node.getblock(prev_hash)
        nbits = int(prev_block["bits"], 16)
        prev_time = prev_block["time"]

        stake_block_hash = node.gettransaction(unspent["txid"])["blockhash"]
        stake_time = node.getblock(stake_block_hash)["time"]

        ntime = self.find_ntime(prev_hash, prev_height, prev_time, nbits, stake_block_hash, stake_time, amount, prevout)

        node.setmocktime(ntime)
        assert node.walletstaking(True)
        self.wait_until(lambda: node.getblockcount() == prev_height + 1)
        assert_equal(node.getblockcount(), prev_height + 1)
        assert not node.walletstaking(False)

        block = node.getblock(node.getbestblockhash(), 2)
        coinstake_txid = block["tx"][1]["txid"]
        reward_value = block["tx"][1]["vout"][1]["value"]

        dest = node.getnewaddress()
        rawtx = node.createrawtransaction(
            [{"txid": coinstake_txid, "vout": 1}],
            {dest: Decimal(reward_value) - Decimal("0.01")},
        )
        signed = node.signrawtransactionwithwallet(rawtx)["hex"]

        # Spending before maturity should be rejected
        assert_raises_rpc_error(
            -26,
            "bad-txns-premature-spend-of-coinbase",
            lambda: node.sendrawtransaction(signed),
        )

        node.setmocktime(0)
        node.generatetoaddress(COINBASE_MATURITY, addr)

        # Now the spend should be accepted
        spend_txid = node.sendrawtransaction(signed)
        assert spend_txid in node.getrawmempool()


if __name__ == "__main__":
    CoinMaturityTest(__file__).main()
