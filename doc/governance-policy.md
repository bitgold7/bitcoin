# Governance and Operational Policy

This document describes governance concepts for derivative networks based on this codebase. Bitcoin Core itself does not implement staking, dividend payouts, or the infrastructure listed below. Operators of downstream projects should adapt these guidelines to their own rulesets.

## Network infrastructure references

- **DNS seeders:** provide multiple independent seeders, e.g. `seed1.example.org`, `seed2.example.org`.
- **Block explorers:** publish at least one explorer endpoint such as `https://explorer.example.org`.
- **Bootstrap archives:** offer signed bootstrap data at `https://bootstrap.example.org` or via torrents.

Replace the above with the actual locations maintained by the project.

## Deterministic builds and release signing

Reproducible binaries should be built with the scripts in [`contrib/guix/`](../contrib/guix/). Maintainers sign release artifacts using `gpg --detach-sign`, publishing signatures alongside binaries for verification.

## Logo and ticker assets

Projects may define an official ticker (e.g. `XYZ`) and ship logo files in `share/` for wallets or explorers. Assets are not included in this repository.

## Staking, dividends, and governance rules

If a derivative network adds staking or dividend mechanisms, the following policy outline may serve as a starting point:

- Staking rewards and dividend rates are set by consensus and announced prior to activation.
- Distribution schedules and any treasury arrangements must be transparent and auditable.
- Parameter changes require community review and sign-off through the project's chosen governance process.

