#!/usr/bin/env python3
"""Test wallet dividend RPCs."""
from decimal import Decimal

from test_framework.test_framework import BitcoinTestFramework
from test_framework.blocktools import create_block, create_coinbase
from test_framework.messages import (
    COIN,
    COutPoint,
    CTransaction,
    CTxIn,
    CTxOut,
    hash256,
    uint256_from_compact,
)
from test_framework.script import CScript, OP_TRUE
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

class WalletDividendTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def run_test(self):
        node = self.nodes[0]
        addr = node.getnewaddress()
        info = node.getwalletdividends()
        assert_equal(info, {})
        claimed = node.claimwalletdividends()
        assert_equal(claimed, {})

        # Mine some blocks to obtain a staking coin
        node.generatetoaddress(110, addr)
        unspent = node.listunspent()[0]
        txid = unspent["txid"]
        vout = unspent["vout"]
        amount = int(Decimal(unspent["amount"]) * COIN)
        script = CScript(bytes.fromhex(unspent["scriptPubKey"]))

        prev_height = node.getblockcount()
        prev_hash = node.getbestblockhash()
        prev_block = node.getblock(prev_hash)
        nbits = int(prev_block["bits"], 16)
        prev_time = prev_block["time"]

        stake_block_hash = node.gettransaction(txid)["blockhash"]
        stake_time = node.getblock(stake_block_hash)["time"]
        prevout = {"txid": txid, "vout": vout}

        global STAKE_TIMESTAMP_MASK
        STAKE_TIMESTAMP_MASK = node.getblockchaininfo()["pos_timestamp_mask"]

        ntime = stake_time + MIN_STAKE_AGE
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

        reward = 50 * COIN
        validator_reward = reward * 9 // 10
        dividend_reward = reward - validator_reward

        coinstake = CTransaction()
        coinstake.nLockTime = ntime
        coinstake.vin.append(CTxIn(COutPoint(int(txid, 16), vout)))
        coinstake.vout.append(CTxOut(0, CScript()))
        coinstake.vout.append(CTxOut(amount + validator_reward, script))
        coinstake.vout.append(CTxOut(dividend_reward, CScript([OP_TRUE])))
        # Intentionally leave coinstake unsigned to produce an invalid block

        coinbase = create_coinbase(prev_height + 1, nValue=0)
        block = create_block(
            int(prev_hash, 16),
            coinbase,
            ntime,
            tmpl={"bits": prev_block["bits"], "height": prev_height + 1},
            txlist=[coinstake],
        )
        block.hashMerkleRoot = block.calc_merkle_root()

        pool_before = node.getdividendpool()["amount"]
        node.submitblock(block.serialize().hex())
        assert_equal(node.getblockcount(), prev_height)
        assert_equal(node.getdividendpool()["amount"], pool_before)

        node.setmocktime(0)

if __name__ == '__main__':
    WalletDividendTest(__file__).main()
