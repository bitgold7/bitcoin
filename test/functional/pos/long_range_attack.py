"""Attempt a long-range attack by forking from deep history."""

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

STAKE_TIMESTAMP_MASK = 0xF
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


class PosLongRangeAttackTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        node = self.nodes[0]
        addr = node.getnewaddress()
        node.generatetoaddress(200, addr)

        prev_height = node.getblockcount()
        fork_height = prev_height - 10
        fork_hash = node.getblockhash(fork_height)
        fork_block = node.getblock(fork_hash)
        nbits = int(fork_block["bits"], 16)
        prev_time = fork_block["time"]

        # use a stake that existed before the fork height
        stake = next(u for u in node.listunspent() if u["confirmations"] > 20)
        txid = stake["txid"]
        vout = stake["vout"]
        amount = int(stake["amount"] * COIN)
        prevout = {"txid": txid, "vout": vout}
        stake_hash = node.gettransaction(txid)["blockhash"]
        stake_time = node.getblock(stake_hash)["time"]

        ntime = prev_time + 16
        while not check_kernel(
            fork_hash,
            fork_height,
            prev_time,
            nbits,
            stake_hash,
            stake_time,
            amount,
            prevout,
            ntime,
        ):
            ntime += 16

        script = CScript(bytes.fromhex(stake["scriptPubKey"]))
        cs = CTransaction()
        cs.nLockTime = ntime
        cs.vin.append(CTxIn(COutPoint(int(txid, 16), vout)))
        cs.vout.append(CTxOut(0, CScript()))
        reward = 50 * COIN
        cs.vout.append(CTxOut(amount + reward, script))
        signed_hex = node.signrawtransactionwithwallet(cs.serialize().hex())["hex"]
        cs = CTransaction()
        cs.deserialize(bytes.fromhex(signed_hex))

        coinbase = create_coinbase(fork_height + 1, nValue=0)
        block = create_block(
            int(fork_hash, 16),
            coinbase,
            ntime,
            tmpl={"bits": fork_block["bits"], "height": fork_height + 1},
            txlist=[cs],
        )
        block.hashMerkleRoot = block.calc_merkle_root()
        assert node.submitblock(block.serialize().hex()) is not None
        assert_equal(node.getblockcount(), prev_height)


if __name__ == "__main__":
    PosLongRangeAttackTest(__file__).main()
