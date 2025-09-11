# Bulletproofs

Bitcoin Core includes experimental support for [Bulletproofs](https://eprint.iacr.org/2017/1066),
which allow transaction amounts to be kept confidential while remaining
verifiable by anyone.

## Enabling Bulletproofs

Bulletproof functionality is disabled by default. To compile Bitcoin Core with
Bulletproof support, configure the build system with the
`ENABLE_BULLETPROOFS` option:

```bash
cmake -DENABLE_BULLETPROOFS=ON <path-to-source>
```

If the required dependencies are not available, Bulletproofs will remain
disabled and the related RPC methods will return an error when called.

## Privacy guarantees and limitations

Bulletproofs hide transaction amounts but do **not** conceal sender and
receiver addresses or the transaction graph. Transactions using
Bulletproofs are still linkable on-chain and should not be considered a
complete privacy solution.  The current implementation is experimental
and should not be used with real funds.

## RPC interface

Two RPC methods expose the Bulletproof functionality:

* `createrawbulletprooftransaction inputs outputs`
  – Creates a raw transaction with Bulletproof range proofs.
* `verifybulletproof proof`
  – Verifies a Bulletproof and returns whether it is valid.

Both methods are available through `bitcoin-cli` when Bitcoin Core is
compiled with Bulletproof support.

