# BitGold Whitepaper Lite

This document provides a high-level overview of BitGold's protocol design. It is intended as a concise reference describing the core mechanisms that govern consensus, incentives, and transaction processing.

## Proof of Stake

BitGold secures the network through a proof-of-stake (PoS) consensus algorithm. Participants lock BGD coins in staking outputs and take turns creating blocks proportionally to the amount staked. This design reduces energy consumption compared to proof-of-work while maintaining decentralized security. Stakeholders who help secure the network collect staking rewards and transaction fees.

## Dividend Policy

In addition to staking rewards, BitGold distributes a dividend from block subsidies to long‑term holders. Dividend outputs mature after a fixed holding period and can be claimed using standard wallet software. This mechanism incentivizes saving and broad participation in the network.

## Priority Policy

Transactions are prioritized according to a mempool policy that weighs fee rate, coin age, and dividend eligibility. High‑priority transactions are relayed and mined first, while low‑priority transactions may be deferred during congestion. Operators can tune policy parameters to balance fairness and network throughput.

For the full protocol specification, refer to the complete whitepaper and accompanying technical documentation.
