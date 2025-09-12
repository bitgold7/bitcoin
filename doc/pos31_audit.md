# PoS v3.1 Audit

This document summarizes the current state of proof‑of‑stake (PoS) related code in the BitGold repository and highlights areas still needing work in order to fully mirror Blackcoin's PoS v3.1 (BPoS) implementation.

## Existing PoS Code

The repository already contains a sizeable PoS implementation:

* **Consensus parameters** – `src/consensus/params.h` defines `fEnablePoS`, `posActivationHeight`, `nStakeTimestampMask`, `nStakeMinAge`, `nStakeModifierInterval`, `nStakeModifierVersion`, `posLimit` and `nStakeTargetSpacing`.
* **Stake checking** – `src/consensus/pos/stake.cpp` and `src/consensus/pos/stake.h` implement
  * `CheckStakeKernelHash` (stake kernel creation and hashing)
  * `ContextualCheckProofOfStake` (coinstake structure, coin age, timestamp/slot rules)
  * `CheckStakeTimestamp` helper enforcing the 16‑second mask and tight future drift
  * `IsProofOfStake` utility
  * constant `MIN_STAKE_AGE` (8h); target spacing is configured via consensus parameter `nStakeTargetSpacing`
  * minimum stake age, coinstake format, difficulty retargeting, and block signature validation are enforced during block checks
* **Stake modifier handling** – `src/consensus/pos/stakemodifier.cpp` provides modifier computation and `src/node/stake_modifier_manager.cpp` tracks and caches modifiers for validation and P2P.
* **Difficulty retargeting** – `src/consensus/pos/difficulty.cpp` supplies a PoS retarget routine used by `GetNextWorkRequired`.
* **Wallet staking** – `src/wallet/bitgoldstaker.cpp` contains a staking loop that constructs and signs coinstakes and submits PoS blocks.
* **Network protocol support** – a new P2P message type (`STAKEMODIFIER`) is declared in `src/protocol.h` and handled throughout `net.cpp`, `protocol.cpp`, and `net_processing.cpp`.
* **Chain parameter wiring** – `src/kernel/chainparams.cpp` and `src/chainparams.cpp` expose `-posactivationheight` and set `nStakeTimestampMask` for various networks.
* **Tests and docs** – multiple functional tests (`test/functional/feature_posv3.py`, `test/functional/pos_block_staking.py`, `test/functional/pos_reorg.py`) and user documentation (`doc/staking.md`, `doc/bitgoldstaking.md`, `doc/pos3.1-overview.md`, etc.) already exist.

## Status Update

The previously noted gaps against Blackcoin's PoS v3.1 have been addressed:

* A dedicated `ThreadStakeMiner` thread now drives wallet staking, matching upstream behaviour.
* Staking is explicitly gated by the `-staking` (alias `-staker`) command line flag.

With these pieces in place, the repository provides a complete PoS v3.1 implementation.

