# PoS v3.1 Audit

This document summarizes the current state of proof‑of‑stake (PoS) related code in the BitGold repository and highlights areas still needing work in order to fully mirror Blackcoin's PoS v3.1 (BPoS) implementation.

## Existing PoS Code

The repository already contains a sizeable PoS implementation:

* **Consensus parameters** – `src/consensus/params.h` defines `fEnablePoS`, `posActivationHeight`, `nStakeTimestampMask`, `nStakeMinAge`, `nStakeModifierInterval`, `posLimit` and `nStakeTargetSpacing`.
* **Stake checking** – `src/pos/stake.cpp` and `src/pos/stake.h` implement
  * `CheckStakeKernelHash` (stake kernel creation and hashing)
  * `ContextualCheckProofOfStake` (coinstake structure, coin age, timestamp/slot rules)
  * `CheckStakeTimestamp` helper enforcing the 16‑second mask and tight future drift
  * `IsProofOfStake` utility
  * constant `MIN_STAKE_AGE` (1h); target spacing is configured via consensus parameter `nStakeTargetSpacing`
* **Stake modifier handling** – `src/pos/stakemodifier.cpp` and `src/pos/stakemodifier_manager.cpp` provide modifier computation and a manager refreshing it at fixed intervals.
* **Difficulty retargeting** – `src/pos/difficulty.cpp` supplies a PoS retarget routine used by `GetNextWorkRequired`.
* **Wallet staking** – `src/wallet/bitgoldstaker.cpp` contains a staking loop that constructs and signs coinstakes and submits PoS blocks.
* **Network protocol support** – new P2P message types (`COINSTAKE`, `STAKEMODIFIER`) are declared in `src/protocol.h` and handled throughout `net.cpp`, `protocol.cpp`, and `net_processing.cpp`.
* **Chain parameter wiring** – `src/kernel/chainparams.cpp` and `src/chainparams.cpp` expose `-posactivationheight` and set `nStakeTimestampMask` for various networks.
* **Tests and docs** – multiple functional tests (`test/functional/feature_posv3.py`, `test/functional/pos_block_staking.py`, `test/functional/pos_reorg.py`) and user documentation (`doc/staking.md`, `doc/bitgoldstaking.md`, `doc/pos3.1-overview.md`, etc.) already exist.

## Status Update

The previously noted gaps against Blackcoin's PoS v3.1 have been addressed:

* A dedicated `ThreadStakeMiner` thread now drives wallet staking, matching upstream behaviour.
* Staking is explicitly gated by the `-staking` (alias `-staker`) command line flag.

With these pieces in place, the repository provides a complete PoS v3.1 implementation.

