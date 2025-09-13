#!/usr/bin/env bash
#
# Simple smoke test that a new signet instance can be bootstrapped and
# accepts a block solving the signet challenge.

set -e

DATADIR="${BASE_SCRATCH_DIR}/signet-bootstrap"
rm -rf "$DATADIR"
mkdir -p "$DATADIR"

"${BASE_OUTDIR}/bin/bitgoldd" --signet --signetchallenge=51 -datadir="$DATADIR" -daemon

# Wait for the node to be ready
"${BASE_OUTDIR}/bin/bitgold-cli" --signet --signetchallenge=51 -datadir="$DATADIR" -rpcwait getblockchaininfo > /dev/null

# Generate a block using the signet challenge and submit it
BLOCK_HEX=$("${BASE_OUTDIR}/bin/bitgold-cli" --signet --signetchallenge=51 -datadir="$DATADIR" generateblock "raw(51)" '[]' false | jq -r '.hex')
"${BASE_OUTDIR}/bin/bitgold-cli" --signet --signetchallenge=51 -datadir="$DATADIR" submitblock "$BLOCK_HEX" > /dev/null

# Ensure the block was accepted
test "$("${BASE_OUTDIR}/bin/bitgold-cli" --signet --signetchallenge=51 -datadir="$DATADIR" getblockcount)" -eq 1

"${BASE_OUTDIR}/bin/bitgold-cli" --signet --signetchallenge=51 -datadir="$DATADIR" stop > /dev/null
