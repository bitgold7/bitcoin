# BitGold vs. Bitcoin Core

## Consensus
BitGold replaces proof-of-work with PoS v3, activating at height 2. Block rewards follow a 50,000‑block halving schedule with an 8 million BGD cap and a 3 million BGD premine. A quarterly dividend redistributes a portion of block subsidies to long‑term stakeholders.

## Transactions
Confidential transactions using Bulletproof commitments are supported and can be signaled via version bit 3. Wallets may create shielded outputs and verify range proofs, features absent in upstream Bitcoin Core.

## Policy
The mempool still enforces ancestor and descendant limits but prioritizes transactions by fee rate, coin age, and dividend eligibility. Nodes can optionally pay out accumulated dividends via a `-dividendpayouts` flag.

## Network Parameters
Mainnet uses port 8888, different message start bytes, and custom address prefixes. Base58 addresses beginning with **B** and **G** replace Bitcoin’s **1** and **3** prefixes.

## Rationale
These deviations aim to provide energy‑efficient security, incentivize long‑term holding, and experiment with confidential transactions while remaining close enough to Bitcoin Core for ease of maintenance.
