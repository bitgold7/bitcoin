# Merkle proof benchmarks

Micro-benchmarks exercising Merkle proof generation and verification live in
`src/bench/merkle_proof.cpp`. They generate a tree of 1024 random leaves and
measure:

* proof generation for random leaves;
* cached verification of a batch of proofs, using `MERKLE_PROOF_CACHE` entries;
* multi-threaded verification with the number of workers controlled by
  `MERKLE_PROOF_THREADS`.

## Configuration

Environment variables allow tuning limits:

* `MERKLE_PROOF_CACHE` (default 128) sets the maximum number of cached proof
  results. When the cache grows beyond this value it is cleared.
* `MERKLE_PROOF_THREADS` (default hardware concurrency) sets the number of
  threads used in the multi-threaded verification benchmark.

## Resource usage

Each proof over 1024 leaves is roughly 320 bytes. Caching 128 proofs consumes
around 40&nbsp;KiB of memory. Multi-threaded verification scales with the number of
threads up to available cores.
