# Bulletproof Commitment Serialization

This document specifies the canonical serialization format for Bulletproof commitments and proofs used by descriptor expressions and the Bitcoin Core codebase.

## Commitments

Bulletproof commitments are serialized using the compressed secp256k1 representation:

* 33 bytes in total.
* The first byte encodes the sign of the Y coordinate (as with `secp256k1_ec_pubkey_serialize` using `SECP256K1_EC_COMPRESSED`).
* The remaining 32 bytes encode the X coordinate in big-endian form.

Descriptor expressions embed commitments as hex strings using this 33‑byte encoding.

## Proofs

Bulletproof range proofs are serialized as raw byte strings as returned by `secp256k1_bulletproof_rangeproof_serialize`.

* The proof is encoded as a length-prefixed blob: first a [compact size](https://en.bitcoin.it/wiki/Protocol_documentation#Variable_length_integer) indicating the proof length, followed by the proof bytes.
* When represented in descriptors or external APIs, the entire length‑prefixed blob is hex encoded.

## Descriptor usage

A Bulletproof commitment/proof pair can be represented in descriptors using the following function:

```
bpv(<commitment_hex>,<proof_hex>)
```

Both arguments are hex strings corresponding to the serialized commitment and the length‑prefixed proof respectively.

## Examples

```
# Commitment and proof encoded as hex
bpv(08f...<33 bytes>...,fdff01...
```

## See also

* `contrib/bulletproof/encoding_examples.py` provides reference encoding and decoding helpers in Python.
