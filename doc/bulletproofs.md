# Bulletproofs

BitGold includes experimental support for [Bulletproofs](https://eprint.iacr.org/2017/1066),
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

## Activation and Usage

Bulletproof support is compiled into BitGold by default.  The feature is
deployed on mainnet and testnet via BIP9 using version bit 3.  Signaling
begins on **January 1st, 2025** and, if the threshold is met, enforcement
begins after a confirmation period.  The deployment times out on
**January 1st, 2026**.

- **Mainnet**: 2016-block periods with a 90% threshold (1815 blocks)
- **Testnet**: 2016-block periods with a 75% threshold (1512 blocks)
- **Signet/Regtest**: always active for testing

Nodes without Bulletproof support will reject transactions that set the
Bulletproof version bit once the deployment is active.  The deployment
parameters may be overridden for testing with the `-vbparams`
command-line argument (see `-help-debug`).

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

Both methods are available through `bitgold-cli` when BitGold is
compiled with Bulletproof support.  Wallets can also create Bulletproof
transactions using standard RPCs such as `sendtoaddress` once the feature
is activated on the network.

### Example usage

```
bitgold-cli createwallet bpwallet
bitgold-cli -rpcwallet=bpwallet getnewaddress
bitgold-cli createrawbulletprooftransaction "[]" "{\"data\":\"00\"}"
```

The resulting hex string contains the serialized commitment and proof as
described in `doc/bulletproof-serialization.md`.

