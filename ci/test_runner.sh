#!/usr/bin/env bash
set -e
export LC_ALL=C.UTF-8

# Run functional tests including the dividend hybrid suite.
"$(dirname "$0")/test_run_all.sh" "$@"
