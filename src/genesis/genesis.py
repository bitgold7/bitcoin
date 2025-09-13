#!/usr/bin/env python3
"""Utility script to reproduce the BitGold genesis block parameters."""

import hashlib
import struct


def varint(n: int) -> bytes:
    """Encode an integer as Bitcoin's compact size."""
    if n < 0xfd:
        return bytes([n])
    if n <= 0xFFFF:
        return b"\xfd" + struct.pack("<H", n)
    if n <= 0xFFFFFFFF:
        return b"\xfe" + struct.pack("<I", n)
    return b"\xff" + struct.pack("<Q", n)


def sha256d(b: bytes) -> bytes:
    """Double SHA256."""
    return hashlib.sha256(hashlib.sha256(b).digest()).digest()


def create_coinbase(psz: str, reward: int) -> bytes:
    psz_b = psz.encode()
    script_sig = (
        bytes([len(struct.pack("<I", 486604799))])
        + struct.pack("<I", 486604799)
        + bytes([1])
        + b"\x04"
        + bytes([len(psz_b)])
        + psz_b
    )
    script_pubkey = bytes.fromhex(
        "41"
        + "04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61"
          "deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f"
        + "ac"
    )
    tx = (
        struct.pack("<I", 1)
        + b"\x01"
        + b"\x00" * 32
        + struct.pack("<I", 0xFFFFFFFF)
        + varint(len(script_sig))
        + script_sig
        + struct.pack("<I", 0xFFFFFFFF)
        + b"\x01"
        + struct.pack("<Q", reward)
        + varint(len(script_pubkey))
        + script_pubkey
        + struct.pack("<I", 0)
    )
    return tx


def mine_genesis(n_time: int, n_bits: int, psz: str, reward: int):
    coinbase = create_coinbase(psz, reward)
    merkle = sha256d(coinbase)
    target = (n_bits & 0xFFFFFF) * 2 ** (8 * ((n_bits >> 24) - 3))
    nonce = 0
    while True:
        header = (
            struct.pack("<I", 1)
            + b"\x00" * 32
            + merkle
            + struct.pack("<I", n_time)
            + struct.pack("<I", n_bits)
            + struct.pack("<I", nonce)
        )
        h = sha256d(header)
        if int.from_bytes(h[::-1], "big") <= target:
            return nonce, merkle[::-1].hex(), h[::-1].hex()
        nonce += 1


if __name__ == "__main__":
    TIMESTAMP = "The Times 01/Jan/2024 BitGold unveils the digital gold standard"
    NONCE, MERKLE, HASH = mine_genesis(
        1704067200,
        0x1e0ffff0,
        TIMESTAMP,
        3_000_000 * 100_000_000,
    )
    print("pszTimestamp:", TIMESTAMP)
    print("nonce:", NONCE)
    print("merkle root:", MERKLE)
    print("genesis hash:", HASH)

