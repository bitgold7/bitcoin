"""Verify staking rewards are clipped at the configured coin-age weight cap."""

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
MAX_AGE_WEIGHT = 60 * 60 * 24 * 30


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


class PosRewardClippingTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        node = self.nodes[0]
        global STAKE_TIMESTAMP_MASK
        STAKE_TIMESTAMP_MASK = node.getblockchaininfo()["pos_timestamp_mask"]
        addr = node.getnewaddress()
        node.generatetoaddress(110, addr)

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

        coin_age = MAX_AGE_WEIGHT + 24 * 60 * 60
        ntime = stake_time + coin_age
        node.setmocktime(ntime + 60)
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

        subsidy = 50 * COIN
        reward_capped = subsidy * (MAX_AGE_WEIGHT // MIN_STAKE_AGE)
        reward_uncapped = subsidy * (coin_age // MIN_STAKE_AGE)

        script = CScript(bytes.fromhex(unspent["scriptPubKey"]))

        def build_coinstake(reward):
            tx = CTransaction()
            tx.nLockTime = ntime
            tx.vin.append(CTxIn(COutPoint(int(txid, 16), vout)))
            tx.vout.append(CTxOut(0, CScript()))
            tx.vout.append(CTxOut(amount + reward, script))
            signed_hex = node.signrawtransactionwithwallet(tx.serialize().hex())["hex"]
            tx = CTransaction()
            tx.deserialize(bytes.fromhex(signed_hex))
            return tx

        bad_coinstake = build_coinstake(reward_uncapped)
        coinbase = create_coinbase(prev_height + 1, nValue=0)
        bad_block = create_block(
            int(prev_hash, 16),
            coinbase,
            ntime,
            tmpl={"bits": prev_block["bits"], "height": prev_height + 1},
            txlist=[bad_coinstake],
        )
        bad_block.hashMerkleRoot = bad_block.calc_merkle_root()
        assert node.submitblock(bad_block.serialize().hex()) is not None
        assert_equal(node.getblockcount(), prev_height)

        good_coinstake = build_coinstake(reward_capped)
        good_block = create_block(
            int(prev_hash, 16),
            coinbase,
            ntime,
            tmpl={"bits": prev_block["bits"], "height": prev_height + 1},
            txlist=[good_coinstake],
        )
        good_block.hashMerkleRoot = good_block.calc_merkle_root()
        node.submitblock(good_block.serialize().hex())
        assert_equal(node.getblockcount(), prev_height + 1)


if __name__ == "__main__":
    PosRewardClippingTest(__file__).main()
