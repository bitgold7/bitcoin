#!/usr/bin/env bash
#
# Simple smoke test that a new signet instance can be bootstrapped.

set -e

DATADIR="${BASE_SCRATCH_DIR}/signet-bootstrap"
rm -rf "$DATADIR"
mkdir -p "$DATADIR"

"${BASE_OUTDIR}/bin/bitgoldd" -signet -datadir="$DATADIR" -daemon

"${BASE_OUTDIR}/bin/bitgold-cli" -signet -datadir="$DATADIR" -rpcwait getblockchaininfo > /dev/null
"${BASE_OUTDIR}/bin/bitgold-cli" -signet -datadir="$DATADIR" stop > /dev/null
