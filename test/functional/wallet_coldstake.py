#!/usr/bin/env python3
from test_framework.blocktools import COINBASE_MATURITY
from test_framework.messages import COIN, hash256, uint256_from_compact
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

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


class WalletColdStakeTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def run_test(self):
        node = self.nodes[0]
        global STAKE_TIMESTAMP_MASK
        STAKE_TIMESTAMP_MASK = node.getblockchaininfo()["pos_timestamp_mask"]

        owner = node.getnewaddress()
        staker = node.getnewaddress()
        res = node.delegatestakeaddress(owner, staker)
        cold_addr = res["address"]
        script = res["script"]
        node.registercoldstakeaddress(cold_addr, script)
        info = node.getaddressinfo(cold_addr)
        assert info["iswatchonly"]

        node.generatetoaddress(COINBASE_MATURITY + 1, cold_addr)

        unspent = next(
            u
            for u in node.listunspent()
            if u["address"] == cold_addr and u["confirmations"] >= COINBASE_MATURITY
        )
        txid = unspent["txid"]
        vout = unspent["vout"]
        amount = int(unspent["amount"] * COIN)
        prevout = {"txid": txid, "vout": vout}

        prev_height = node.getblockcount()
        prev_hash = node.getbestblockhash()
        prev_block = node.getblock(prev_hash)
        nbits = int(prev_block["bits"], 16)
        prev_time = prev_block["time"]

        stake_block_hash = node.gettransaction(txid)["blockhash"]
        stake_time = node.getblock(stake_block_hash)["time"]

        ntime = prev_time + 16
        while not check_kernel(
            prev_hash,
            prev_height,
            prev_time,
            nbits,
            stake_block_hash,
            stake_time,
            amount,
            prevout,
            ntime,
        ):
            ntime += 16

        node.setmocktime(ntime)
        assert node.walletstaking(True)
        self.wait_until(lambda: node.getblockcount() == prev_height + 1)
        blockhash = node.getbestblockhash()
        assert not node.walletstaking(False)

        block = node.getblock(blockhash, 2)
        reward_address = block["tx"][1]["vout"][1]["scriptPubKey"]["address"]
        assert_equal(reward_address, owner)
        owner_unspents = [u for u in node.listunspent() if u.get("address") == owner]
        staker_unspents = [u for u in node.listunspent() if u.get("address") == staker]
        assert owner_unspents
        assert not staker_unspents


if __name__ == "__main__":
    WalletColdStakeTest(__file__).main()
