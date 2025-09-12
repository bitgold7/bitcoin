# Reproducible Builds

This document outlines how Bitcoin Core can be built reproducibly using the
Guix-based build system shipped in `contrib/guix`.

## Environment

* GNU Guix installed on the host.
* At least 16GB of free space for the `/gnu/store` and 8GB per target
  architecture.
* Two independent operators to perform the build and provide attestations.

## Build Steps

1. Clone the repository and ensure the work tree is clean.
2. From the repository root run:
   ```sh
   ./contrib/guix/guix-build
   ```
   The script creates deterministic binaries for all supported platforms.
3. Each operator generates attestation data and SHA256 sums:
   ```sh
   env GUIX_SIGS_REPO=/path/to/guix.sigs \
       SIGNER=<gpg-key-name> ./contrib/guix/guix-attest
   ```
4. Operators sign the resulting `all.SHA256SUMS` files with their release key:
   ```sh
   gpg --detach-sign --armor all.SHA256SUMS
   ```

## Verification

A build is considered reproducible once the outputs from at least two operators
match.  To verify this, collect their signatures in `guix.sigs` and run:
```sh
env GUIX_SIGS_REPO=/path/to/guix.sigs ./contrib/guix/guix-verify
```

## Automation

The helper script `contrib/guix/guix-publish` verifies the attestations from two
operators, combines their SHA256 sums, signs the result and writes
`SHA256SUMS` and `SHA256SUMS.asc` to a release directory.  The GitHub workflow
`guix.yml` runs these steps automatically and publishes the signed hashes as
build artifacts.
