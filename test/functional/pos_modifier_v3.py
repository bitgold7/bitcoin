#!/usr/bin/env python3
"""Test staking with stake modifier version 3 and rejection of invalid PoS blocks."""

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
MIN_STAKE_AGE = 60 * 60

def compute_modifier(prev_modifier, prev_hash, prev_height, prev_time):
    data = (
        prev_modifier
        + bytes.fromhex(prev_hash)[::-1]
        + prev_height.to_bytes(4, "little")
        + prev_time.to_bytes(4, "little")
    )
    return hash256(data)

def check_kernel(prev_modifier, prev_hash, prev_height, prev_time, nbits,
                 stake_hash, stake_time, amount, prevout, ntime):
    if ntime & STAKE_TIMESTAMP_MASK:
        return False, prev_modifier
    if ntime <= stake_time or ntime - stake_time < MIN_STAKE_AGE:
        return False, prev_modifier
    stake_modifier = compute_modifier(prev_modifier, prev_hash, prev_height, prev_time)
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
    return int.from_bytes(proof[::-1], "big") <= target, stake_modifier

class PosModifierV3Test(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        node = self.nodes[0]
        global STAKE_TIMESTAMP_MASK
        STAKE_TIMESTAMP_MASK = node.getblockchaininfo()["pos_timestamp_mask"]

        addr = node.getnewaddress()
        node.generatetoaddress(200, addr)

        # select two mature utxos for staking
        unspents = sorted(node.listunspent(), key=lambda x: x["confirmations"], reverse=True)
        utxo1, utxo2 = unspents[0], unspents[1]

        modifier = b"\x00" * 32

        def stake_block(prev_modifier, utxo):
            txid = utxo["txid"]
            vout = utxo["vout"]
            amount = int(utxo["amount"] * COIN)
            prevout = {"txid": txid, "vout": vout}

            prev_height = node.getblockcount()
            prev_hash = node.getbestblockhash()
            prev_block = node.getblock(prev_hash)
            nbits = int(prev_block["bits"], 16)
            prev_time = prev_block["time"]

            stake_block_hash = node.gettransaction(txid)["blockhash"]
            stake_time = node.getblock(stake_block_hash)["time"]

            ntime = prev_time + 16
            while True:
                ok, new_modifier = check_kernel(
                    prev_modifier,
                    prev_hash,
                    prev_height,
                    prev_time,
                    nbits,
                    stake_block_hash,
                    stake_time,
                    amount,
                    prevout,
                    ntime,
                )
                if ok:
                    break
                ntime += 16

            script = CScript(bytes.fromhex(utxo["scriptPubKey"]))
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
            return block, new_modifier

        # stake first valid block
        prev_height = node.getblockcount()
        block1, modifier = stake_block(modifier, utxo1)
        node.submitblock(block1.serialize().hex())
        assert_equal(node.getblockcount(), prev_height + 1)

        # attempt second block with wrong modifier
        prev_height = node.getblockcount()
        bad_block, _ = stake_block(b"\x00" * 32, utxo2)
        assert node.submitblock(bad_block.serialize().hex()) is not None
        assert_equal(node.getblockcount(), prev_height)

        # now stake second block correctly
        good_block, modifier = stake_block(modifier, utxo2)
        node.submitblock(good_block.serialize().hex())
        assert_equal(node.getblockcount(), prev_height + 1)

if __name__ == '__main__':
    PosModifierV3Test(__file__).main()
