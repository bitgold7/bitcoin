Bulletproof Commitments
-----------------------
* Experimental support for Bulletproof range proofs can be enabled at build
  time with `-DENABLE_BULLETPROOFS=ON`.
* Validation rules are gated behind the BIP9 deployment `DEPLOYMENT_BULLETPROOF`
  using version bit 3.  Mainnet and testnet deployments are defined but set to
  `NEVER_ACTIVE`.  Signet and regtest activate the deployment immediately for
  testing.
* Nodes without Bulletproof support will continue to reject transactions that
  set the Bulletproof version bit.
