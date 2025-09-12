#!/usr/bin/env python3
"""Ensure invalid PoS blocks are rejected for timestamp and coinstake rules."""

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

STAKE_TIMESTAMP_MASK = None
STAKE_TARGET_SPACING = None
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


class PosInvalidTimestampTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        node = self.nodes[0]
        global STAKE_TIMESTAMP_MASK, STAKE_TARGET_SPACING
        info = node.getblockchaininfo()
        STAKE_TIMESTAMP_MASK = info["pos_timestamp_mask"]
        STAKE_TARGET_SPACING = info["pos_target_spacing"]
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

        ntime = prev_time + 16
        ntime_invalid = None
        while ntime < prev_time + STAKE_TARGET_SPACING:
            if check_kernel(
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
                ntime_invalid = ntime
                break
            ntime += 16
        assert ntime_invalid is not None

        ntime = max(prev_time + STAKE_TARGET_SPACING, ntime_invalid + 16)
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
        ntime_valid = ntime

        script = CScript(bytes.fromhex(unspent["scriptPubKey"]))

        def make_coinstake(locktime):
            cs = CTransaction()
            cs.nLockTime = locktime
            cs.vin.append(CTxIn(COutPoint(int(txid, 16), vout)))
            cs.vout.append(CTxOut(0, CScript()))
            reward = 50 * COIN
            cs.vout.append(CTxOut(amount + reward, script))
            signed_hex = node.signrawtransactionwithwallet(cs.serialize().hex())["hex"]
            cs = CTransaction()
            cs.deserialize(bytes.fromhex(signed_hex))
            return cs

        coinbase = create_coinbase(prev_height + 1, nValue=0)

        # Case 1: block timestamp too early
        bad_cs = make_coinstake(ntime_invalid)
        block = create_block(
            int(prev_hash, 16),
            coinbase,
            ntime_invalid,
            tmpl={"bits": prev_block["bits"], "height": prev_height + 1},
            txlist=[bad_cs],
        )
        block.hashMerkleRoot = block.calc_merkle_root()
        assert node.submitblock(block.serialize().hex()) is not None
        assert_equal(node.getblockcount(), prev_height)

        # Case 2: coinstake first output non-empty
        bad_cs = make_coinstake(ntime_valid)
        bad_cs.vout[0] = CTxOut(1, script)
        bad_hex = node.signrawtransactionwithwallet(bad_cs.serialize().hex())["hex"]
        bad_cs = CTransaction()
        bad_cs.deserialize(bytes.fromhex(bad_hex))
        block = create_block(
            int(prev_hash, 16),
            coinbase,
            ntime_valid,
            tmpl={"bits": prev_block["bits"], "height": prev_height + 1},
            txlist=[bad_cs],
        )
        block.hashMerkleRoot = block.calc_merkle_root()
        assert node.submitblock(block.serialize().hex()) is not None
        assert_equal(node.getblockcount(), prev_height)

        # Case 3: coinstake timestamp mismatch
        bad_cs = make_coinstake(ntime_valid + 16)
        block = create_block(
            int(prev_hash, 16),
            coinbase,
            ntime_valid,
            tmpl={"bits": prev_block["bits"], "height": prev_height + 1},
            txlist=[bad_cs],
        )
        block.hashMerkleRoot = block.calc_merkle_root()
        assert node.submitblock(block.serialize().hex()) is not None
        assert_equal(node.getblockcount(), prev_height)

        # Case 4: block timestamp not multiple of 16 seconds
        bad_cs = make_coinstake(ntime_valid + 1)
        block = create_block(
            int(prev_hash, 16),
            coinbase,
            ntime_valid + 1,
            tmpl={"bits": prev_block["bits"], "height": prev_height + 1},
            txlist=[bad_cs],
        )
        block.hashMerkleRoot = block.calc_merkle_root()
        assert node.submitblock(block.serialize().hex()) is not None
        assert_equal(node.getblockcount(), prev_height)

        # Case 5: block timestamp too far in the future
        node.setmocktime(prev_time - 100)
        bad_cs = make_coinstake(ntime_valid)
        block = create_block(
            int(prev_hash, 16),
            coinbase,
            ntime_valid,
            tmpl={"bits": prev_block["bits"], "height": prev_height + 1},
            txlist=[bad_cs],
        )
        block.hashMerkleRoot = block.calc_merkle_root()
        assert node.submitblock(block.serialize().hex()) is not None
        assert_equal(node.getblockcount(), prev_height)
        node.setmocktime(0)


if __name__ == "__main__":
    PosInvalidTimestampTest(__file__).main()

