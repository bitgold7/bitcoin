#!/usr/bin/env bash
#
# Simple smoke test that a new signet instance can be bootstrapped.

set -e

DATADIR="${BASE_SCRATCH_DIR}/signet-bootstrap"
rm -rf "$DATADIR"
mkdir -p "$DATADIR"

"${BASE_OUTDIR}/bin/bitcoind" -signet -datadir="$DATADIR" -daemon

"${BASE_OUTDIR}/bin/bitcoin-cli" -signet -datadir="$DATADIR" -rpcwait getblockchaininfo > /dev/null
"${BASE_OUTDIR}/bin/bitcoin-cli" -signet -datadir="$DATADIR" stop > /dev/null
