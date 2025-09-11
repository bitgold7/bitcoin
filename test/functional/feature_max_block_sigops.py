#!/usr/bin/env python3
"""Test mining of a max-weight block with a 100k-sigop transaction and mempool ordering under -bgdpriority."""

from hashlib import sha256

from test_framework.blocktools import COIN, create_coinbase
from test_framework.key import ECKey
from test_framework.messages import (
    COutPoint,
    CTransaction,
    CTxIn,
    CTxInWitness,
    CTxOut,
    MAX_BLOCK_WEIGHT,
    ser_compact_size,
)
from test_framework.script import (
    CScript,
    OP_0,
    OP_CHECKMULTISIG,
    OP_RETURN,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from test_framework.wallet import MiniWallet


class MaxBlockSigopsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.extra_args = [
            ["-acceptnonstdtxn=1"],
            ["-acceptnonstdtxn=1", "-bgdpriority"],
        ]

    def create_redeem_script(self):
        keys = []
        for i in range(20):
            k = ECKey()
            k.set((i + 1).to_bytes(32, "big"), True)
            keys.append(k.get_pubkey().get_bytes())
        return CScript([OP_0] + keys + [20, OP_CHECKMULTISIG])

    def sorted_mempool(self, node):
        mp = node.getrawmempool(True)
        return sorted(mp.keys(), key=lambda txid: (mp[txid]["ancestorcount"], int(txid, 16)))

    def run_test(self):
        n0, n1 = self.nodes
        self.connect_nodes(0, 1)
        wallet = MiniWallet(n0)
        self.generate(n0, 101)
        self.sync_blocks()

        redeem_script = self.create_redeem_script()
        script_hash = sha256(bytes(redeem_script)).digest()
        p2wsh = CScript([OP_0, script_hash])

        utxo = wallet.get_utxo()
        value = int(utxo["value"] * COIN)
        split = CTransaction()
        split.vin = [CTxIn(COutPoint(int(utxo["txid"], 16), utxo["vout"]))]
        amt = value // 5000
        for _ in range(5000):
            split.vout.append(CTxOut(amt, p2wsh))
        wallet.sign_tx(split)
        split_id = n0.sendrawtransaction(split.serialize().hex())
        self.generate(n0, 1)
        self.sync_blocks()

        big = CTransaction()
        for i in range(5000):
            big.vin.append(CTxIn(COutPoint(int(split_id, 16), i)))
            wit = CTxInWitness()
            wit.scriptWitness.stack = [b"", bytes(redeem_script)]
            big.wit.vtxinwit.append(wit)
        big.vout = [CTxOut(0, CScript([OP_RETURN]))]

        height = n0.getblockcount() + 1
        coinbase_w = create_coinbase(height).get_weight()
        header_w = 80 * 4
        target_weight = MAX_BLOCK_WEIGHT - coinbase_w - header_w
        target_vsize = target_weight // 4
        cur_vsize = big.get_vsize()
        pad = target_vsize - cur_vsize
        pad -= len(ser_compact_size(pad)) - 1
        big.vout[0].scriptPubKey = CScript(b"\x6a" + b"\x51" * pad)
        assert_equal(big.get_weight(), target_weight)

        big_hex = big.serialize().hex()
        n0.sendrawtransaction(big_hex)
        n1.sendrawtransaction(big_hex)
        small = wallet.create_self_transfer()["hex"]
        wallet.sendrawtransaction(from_node=n0, tx_hex=small)
        n1.sendrawtransaction(small)
        self.sync_mempools()

        assert_equal(self.sorted_mempool(n0), self.sorted_mempool(n1))
        assert "priority" in n1.getmempoolentry(big.rehash())

        block = self.generateblock(n0, output="raw(42)", transactions=[big_hex])["hash"]
        self.sync_blocks()
        blk = n0.getblock(block)
        assert_equal(blk["weight"], MAX_BLOCK_WEIGHT)
        assert_equal(blk["tx"][1], big.rehash())

        self.generate(n0, 1)


if __name__ == "__main__":
    MaxBlockSigopsTest(__file__).main()
