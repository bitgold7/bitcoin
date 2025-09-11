// Expanding proof-of-stake parameters in consensus Params struct.
struct Params {
    int nStakeMinConfirmations{10}; // Minimum confirmations required for staking
    uint256 posLimitLower; // Lower PoS difficulty limit
    int64_t nMaximumSupply{21000000}; // Maximum coin supply
    // ... other existing parameters
};
