# BitGold Specification

## Proof-of-Stake Rules
BitGold uses PoS v3 with staking enabled from block 2. Blocks must respect an 8-minute target spacing and a timestamp mask of 0xF. Coins become eligible to stake after 8 hours and require 80 confirmations. Stake modifiers refresh every hour using version 3 of the algorithm, and coin-age weight caps after 30 days.

## Emission Mathematics
The network mints 50 BGD per block, halving every 50,000 blocks until the combined subsidy reaches the 8 million BGD supply cap. The genesis block created a 3 million BGD premine, and subsequent subsidies stop once the cap is met.

## Dividend Schedule
One quarter is defined as 16,200 blocks. On each quarter boundary the protocol allocates the dividend pool to staking addresses. Annual rates range from 1% to 10% based on stake age and size, and payouts are prorated if the pool is insufficient.

## Confidential Transaction Format
Confidential transactions embed Pedersen commitments and Bulletproof range proofs. Commitments are serialized as 33-byte compressed secp256k1 points. Each proof is stored as a compact-size length followed by the proof bytes and optional extra data; descriptor APIs use hex strings of these blobs.

## Mempool Policy
Mempool admission follows Bitcoin’s package limits while prioritizing transactions by fee rate, coin age, and dividend eligibility. High‑priority transactions relay and confirm first, whereas low‑priority ones may be delayed under congestion.

## Consensus Parameters
| Parameter | Value |
|-----------|-------|
| Subsidy halving interval | 50,000 blocks |
| Block target spacing | 8 minutes |
| PoS activation height | 2 |
| Stake timestamp mask | 0xF |
| Minimum stake age | 8 hours |
| Stake modifier interval | 1 hour |
| Stake modifier version | 3 |
| Stake confirmations | 80 |
| Max coin-age weight | 30 days |
| Stake target spacing | 8 minutes |
| Maximum supply | 8,000,000 BGD |
| Genesis reward | 3,000,000 BGD |
| Dividend quarter | 16,200 blocks |
| Dividend APR range | 1–10% |
