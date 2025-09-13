# RPC API

## getstakinginfo
Returns whether staking is enabled and if the wallet is currently staking.

*Error codes*: `-8` (invalid parameters)

## reservebalance
Reserve or release balance from staking. Call without arguments to return the current reserved amount.

*Error codes*: `-8` (invalid parameters)

## setstakesplitthreshold
Set or get the stake split threshold. Outputs above this amount are split when staking.

*Error codes*: `-8` (invalid parameters)

## getdividendinfo
Return the current dividend pool, the next payout height, and estimated payouts.

*Error codes*: `-8` (invalid parameters)

## sendshielded
Send to a shielded address using confidential commitments.

*Error codes*: `-5` (invalid address), `-8` (invalid parameters), `-4` (wallet error)

## getshieldedbalance
Return the total balance held in shielded outputs controlled by the wallet.

*Error codes*: `-8` (invalid parameters)

