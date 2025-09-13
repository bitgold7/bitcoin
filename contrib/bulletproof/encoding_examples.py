"""Reference helpers for encoding and decoding Bulletproof commitments and proofs.

This module demonstrates the canonical serialization described in
`doc/bulletproof-serialization.md`.
"""
from __future__ import annotations

from dataclasses import dataclass
import binascii


@dataclass
class BulletproofData:
    commitment: bytes
    proof: bytes

    def encode(self) -> tuple[str, str]:
        """Return hex representations of the commitment and length-prefixed proof."""
        return self.commitment.hex(), self.proof.hex()

    @staticmethod
    def decode(commit_hex: str, proof_hex: str) -> BulletproofData:
        """Create an instance from hex strings."""
        commitment = bytes.fromhex(commit_hex)
        proof = bytes.fromhex(proof_hex)
        return BulletproofData(commitment, proof)


def example() -> None:
    # Example 33-byte commitment (all zeros for illustration)
    commitment = bytes.fromhex("08" + "00" * 32)
    # Example compact size length (0x02) followed by two zero bytes of proof data
    proof = b"\x02\x00\x00"
    bp = BulletproofData(commitment, proof)
    commit_hex, proof_hex = bp.encode()
    print("commitment:", commit_hex)
    print("proof:", proof_hex)
    # Round-trip
    recovered = BulletproofData.decode(commit_hex, proof_hex)
    assert recovered.commitment == commitment
    assert recovered.proof == proof


if __name__ == "__main__":
    example()
