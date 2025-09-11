# Bulletproofs

Bitcoin Core includes experimental support for [Bulletproofs](https://eprint.iacr.org/2017/1066),
short zero‑knowledge range proofs that hide transaction amounts while
allowing anyone to verify that the amounts lie in a valid range.  Each
output reveals only a Pedersen commitment to the amount together with a
Bulletproof proving that the committed value is non‑negative and below
`2^64`.

Bulletproof transactions encode the commitment and proof using a new
`OP_BULLETPROOF` opcode.  A transaction opts in to the feature by setting
the `BULLETPROOF_VERSION` bit in its version field.  Nodes that understand
Bulletproofs verify the range proof and the Pedersen commitment before
accepting the transaction or block.

## Enabling Bulletproofs

Bulletproof functionality is disabled by default. To compile Bitcoin Core with
Bulletproof support, configure the build system with the
`ENABLE_BULLETPROOFS` option:

```bash
cmake -DENABLE_BULLETPROOFS=ON <path-to-source>
```

If the required dependencies are not available, Bulletproofs will remain
disabled and the related RPC methods will return an error when called.
Activation on mainnet and testnet is defined as a BIP9 deployment using
version bit 3.  At the time of writing the deployment parameters are set
to `NEVER_ACTIVE` so Bulletproofs are **not** enforced on those networks.
On signet and regtest the deployment is always active to aid testing.  The
deployment parameters may be overridden for testing with the `-vbparams`
command‑line argument (see `-help-debug`).

## Privacy guarantees and limitations

Bulletproofs hide transaction amounts but do **not** conceal sender and
receiver addresses or the transaction graph. Transactions using
Bulletproofs are still linkable on‑chain and should not be considered a
complete privacy solution.  They do, however, provide strong assurances
that amounts remain confidential while preserving the ability for anyone
to audit total supply.

The current implementation is experimental and should not be used with
real funds.

## RPC interface

Two RPC methods expose the Bulletproof functionality:

* `createrawbulletprooftransaction inputs outputs`
  – Creates a raw transaction with Bulletproof range proofs.  Inputs and
    outputs follow the same structure as `createrawtransaction` but the
    resulting transaction sets the Bulletproof version bit and embeds the
    commitments and proofs.
* `verifybulletproof proof`
  – Verifies a Bulletproof and returns whether it is valid.

Both methods are available through `bitcoin-cli` when Bitcoin Core is
compiled with Bulletproof support.  Wallets can also create Bulletproof
transactions using standard RPCs such as `sendtoaddress` once the feature
is activated on the network.

### Example usage

```
bitcoin-cli createwallet bpwallet
bitcoin-cli -rpcwallet=bpwallet getnewaddress
bitcoin-cli createrawbulletprooftransaction "[]" "{\"data\":\"00\"}"
```

The resulting hex string contains the serialized commitment and proof as
described in `doc/bulletproof-serialization.md`.

