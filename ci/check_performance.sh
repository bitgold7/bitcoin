#!/usr/bin/env bash
# Simple placeholder script to check validation performance regression.
# Requires bench_bitcoin to be built and VALIDATION_BASELINE environment variable set.
set -e
metric=$(./src/bench/bench_bitcoin --filter=ConnectBlock20MB 2>/dev/null | awk '{print $NF}' | tail -n1)
baseline=${VALIDATION_BASELINE:-0}
threshold=${VALIDATION_THRESHOLD:-10}
if [ "$baseline" -ne 0 ]; then
  allowed=$(echo "$baseline*(100+threshold)/100" | bc)
  if (( $(echo "$metric > $allowed" | bc -l) )); then
    echo "Performance regression detected: $metric vs baseline $baseline"
    exit 1
  fi
fi
exit 0
