#!/usr/bin/env python3
"""Ensure blocks violating stake timestamp rules are rejected."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.blocktools import create_block, create_coinbase
from test_framework.messages import (
    CTransaction,
    CTxIn,
    CTxOut,
    COutPoint,
    COIN,
    hash256,
    uint256_from_compact,
)
from test_framework.script import CScript
from test_framework.util import assert_equal

MIN_STAKE_AGE = 60 * 60


def check_kernel(prev_hash, prev_height, prev_time, nbits, stake_hash, stake_time, amount, prevout, ntime, mask):
    if ntime & mask:
        return False
    if ntime <= stake_time or ntime - stake_time < MIN_STAKE_AGE:
        return False
    stake_modifier = hash256(
        bytes.fromhex(prev_hash)[::-1]
        + prev_height.to_bytes(4, "little")
        + prev_time.to_bytes(4, "little")
    )
    ntime_masked = ntime & ~mask
    stake_time_masked = stake_time & ~mask
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


class StakeTimestampTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        node = self.nodes[0]
        info = node.getblockchaininfo()
        mask = info["pos_timestamp_mask"]
        spacing = info["pos_target_spacing"]

        addr = node.getnewaddress()
        node.generatetoaddress(150, addr)

        unspent = node.listunspent()[0]
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

        ntime = (prev_time + spacing) & ~mask
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
            mask,
        ):
            ntime += 16

        script = CScript(bytes.fromhex(unspent["scriptPubKey"]))

        def make_block(ntime):
            coinstake = CTransaction()
            coinstake.nLockTime = ntime
            coinstake.vin.append(CTxIn(COutPoint(int(txid, 16), vout)))
            coinstake.vout.append(CTxOut(0, CScript()))
            reward = 50 * COIN
            coinstake.vout.append(CTxOut(amount + reward, script))
            signed = node.signrawtransactionwithwallet(coinstake.serialize().hex())["hex"]
            coinstake = CTransaction()
            coinstake.deserialize(bytes.fromhex(signed))
            coinbase = create_coinbase(prev_height + 1, nValue=0)
            block = create_block(
                int(prev_hash, 16),
                coinbase,
                ntime,
                tmpl={"bits": prev_block["bits"], "height": prev_height + 1},
                txlist=[coinstake],
            )
            block.hashMerkleRoot = block.calc_merkle_root()
            return block

        # Block failing mask divisibility
        bad_mask_block = make_block(ntime + 1)
        assert_equal(node.submitblock(bad_mask_block.serialize().hex()), "bad-pos-time-mask")
        assert_equal(node.getblockcount(), prev_height)

        # Block failing minimum spacing
        bad_spacing_time = (prev_time & ~mask) + 16
        assert bad_spacing_time < prev_time + spacing
        bad_spacing_block = make_block(bad_spacing_time)
        assert_equal(node.submitblock(bad_spacing_block.serialize().hex()), "bad-pos-time-spacing")
        assert_equal(node.getblockcount(), prev_height)

        # Finally submit a valid block
        good_block = make_block(ntime)
        assert node.submitblock(good_block.serialize().hex()) is None
        assert_equal(node.getblockcount(), prev_height + 1)


if __name__ == "__main__":
    StakeTimestampTest(__file__).main()
