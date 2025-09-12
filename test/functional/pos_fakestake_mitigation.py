#!/usr/bin/env python3
"""Test rejection of coinstake blocks with invalid inputs."""

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
import random

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


class PosFakeStakeMitigationTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def build_block(self, node, inputs, ntime, prev_hash, prev_block, prev_height, script):
        cs = CTransaction()
        cs.nLockTime = ntime
        for utxo in inputs:
            cs.vin.append(CTxIn(COutPoint(int(utxo["txid"], 16), utxo["vout"])))
        cs.vout.append(CTxOut(0, CScript()))
        total_in = sum(int(u["amount"] * COIN) for u in inputs)
        reward = 50 * COIN
        cs.vout.append(CTxOut(total_in + reward, script))
        signed = node.signrawtransactionwithwallet(cs.serialize().hex())["hex"]
        cs = CTransaction()
        cs.deserialize(bytes.fromhex(signed))
        coinbase = create_coinbase(prev_height + 1, nValue=0)
        block = create_block(
            int(prev_hash, 16),
            coinbase,
            ntime,
            tmpl={"bits": prev_block["bits"], "height": prev_height + 1},
            txlist=[cs],
        )
        block.hashMerkleRoot = block.calc_merkle_root()
        return block, cs

    def run_test(self):
        node = self.nodes[0]
        global STAKE_TIMESTAMP_MASK
        STAKE_TIMESTAMP_MASK = node.getblockchaininfo()["pos_timestamp_mask"]
        addr = node.getnewaddress()
        node.generatetoaddress(150, addr)

        unspent = node.listunspent()
        stake = unspent[0]
        other = unspent[1]

        prev_height = node.getblockcount()
        prev_hash = node.getbestblockhash()
        prev_block = node.getblock(prev_hash)
        nbits = int(prev_block["bits"], 16)
        prev_time = prev_block["time"]

        stake_block_hash = node.gettransaction(stake["txid"])["blockhash"]
        stake_time = node.getblock(stake_block_hash)["time"]
        amount = int(stake["amount"] * COIN)
        prevout = {"txid": stake["txid"], "vout": stake["vout"]}

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

        script = CScript(bytes.fromhex(stake["scriptPubKey"]))

        # Duplicate input should be rejected
        block_dup, _ = self.build_block(node, [stake, stake], ntime, prev_hash, prev_block, prev_height, script)
        assert node.submitblock(block_dup.serialize().hex()) is not None
        assert_equal(node.getblockcount(), prev_height)

        # Non-existent input should be rejected
        block_missing, cs_missing = self.build_block(node, [stake, other], ntime, prev_hash, prev_block, prev_height, script)
        cs_missing.vin[1].prevout = COutPoint(random.getrandbits(256), 1)
        block_missing.vtx[1] = cs_missing
        block_missing.hashMerkleRoot = block_missing.calc_merkle_root()
        assert node.submitblock(block_missing.serialize().hex()) is not None
        assert_equal(node.getblockcount(), prev_height)

        # Create a young UTXO with too few confirmations
        txid_low = node.sendtoaddress(addr, 1)
        node.generatetoaddress(1, addr)
        low_conf = next(u for u in node.listunspent() if u["txid"] == txid_low)

        new_prev_height = node.getblockcount()
        new_prev_hash = node.getbestblockhash()
        new_prev_block = node.getblock(new_prev_hash)
        nbits = int(new_prev_block["bits"], 16)
        prev_time = new_prev_block["time"]
        ntime = prev_time + 16
        while not check_kernel(
            new_prev_hash,
            new_prev_height,
            prev_time,
            nbits,
            stake_block_hash,
            stake_time,
            amount,
            prevout,
            ntime,
        ):
            ntime += 16

        block_young, _ = self.build_block(node, [stake, low_conf], ntime, new_prev_hash, new_prev_block, new_prev_height, script)
        assert node.submitblock(block_young.serialize().hex()) is not None
        assert_equal(node.getblockcount(), new_prev_height)


if __name__ == "__main__":
    PosFakeStakeMitigationTest(__file__).main()
