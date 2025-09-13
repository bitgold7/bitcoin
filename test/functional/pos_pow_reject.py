#!/usr/bin/env python3
"""Test that PoW blocks are rejected after activation and PoS blocks are accepted."""

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
from test_framework.util import assert_equal, assert_raises_rpc_error

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


class PosPowRejectTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        # PoS activates at height 202. We start from cache height 200.
        self.extra_args = [["-posactivationheight=202"]]

    def run_test(self):
        node = self.nodes[0]
        global STAKE_TIMESTAMP_MASK
        STAKE_TIMESTAMP_MASK = node.getblockchaininfo()["pos_timestamp_mask"]

        assert_equal(node.getblockcount(), 200)
        addr = node.getnewaddress()

        # Mine block 201 with PoW.
        node.generatetoaddress(1, addr)
        assert_equal(node.getblockcount(), 201)

        # Attempt PoW block 202 and expect rejection.
        assert_raises_rpc_error(-1, "bad-pow", node.generatetoaddress, 1, addr)
        assert_equal(node.getblockcount(), 201)

        # Produce valid PoS block 202.
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

        ntime = max(prev_time + 16, stake_time + MIN_STAKE_AGE)
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

        script = CScript(bytes.fromhex(unspent["scriptPubKey"]))
        coinstake = CTransaction()
        coinstake.nLockTime = ntime
        coinstake.vin.append(CTxIn(COutPoint(int(txid, 16), vout)))
        coinstake.vout.append(CTxOut(0, CScript()))
        reward = 50 * COIN
        coinstake.vout.append(CTxOut(amount + reward, script))
        signed_hex = node.signrawtransactionwithwallet(coinstake.serialize().hex())["hex"]
        coinstake = CTransaction()
        coinstake.deserialize(bytes.fromhex(signed_hex))

        coinbase = create_coinbase(prev_height + 1, nValue=0)
        block = create_block(
            int(prev_hash, 16),
            coinbase,
            ntime,
            tmpl={"bits": prev_block["bits"], "height": prev_height + 1},
            txlist=[coinstake],
        )
        block.hashMerkleRoot = block.calc_merkle_root()
        node.submitblock(block.serialize().hex())
        assert_equal(node.getblockcount(), prev_height + 1)


if __name__ == "__main__":
    PosPowRejectTest(__file__).main()
