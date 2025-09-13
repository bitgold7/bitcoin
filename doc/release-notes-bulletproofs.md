Bulletproof Commitments
-----------------------
* Bulletproof range proof support is now compiled in by default and represents a **consensus change**. Nodes and miners must upgrade before activation to remain on the same chain.
* Validation rules are gated behind the BIP9 deployment `DEPLOYMENT_BULLETPROOF` using version bit 3. Signaling begins on January 1st, 2025 and the deployment times out on January 1st, 2026. Mainnet requires a 90% threshold (1815 of 2016 blocks) while testnet uses a 75% threshold (1512 of 2016 blocks). Signet and regtest activate immediately for testing.
* Once the deployment is active, nodes without Bulletproof support will reject transactions that set the Bulletproof version bit.

