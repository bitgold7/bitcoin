# Staking RPC examples

This document shows how to use RPC calls related to staking and dividends.

## `getstakinginfo`

Returns whether staking is enabled and if the wallet is currently staking.

```
$ bitgold-cli getstakinginfo
{
  "enabled": true,
  "staking": false
}
```

## `setreservebalance`

Reserves a portion of the wallet balance from staking. Call with no
parameters to return the current amount.

```
$ bitgold-cli setreservebalance true 100
{
  "reserved": 100.00000000
}
```

## `reservebalance`

Deprecated alias for `setreservebalance`.

```
$ bitgold-cli reservebalance
{
  "reserved": 100.00000000
}
```

## `setstakesplitthreshold`

Sets the minimum amount before a staking output is split. Setting the value
to `0` disables splitting.

```
$ bitgold-cli setstakesplitthreshold 50
{
  "threshold": 50.00000000
}
```

## `getdividendpool`

Returns the current dividend pool amount.

```
$ bitgold-cli getdividendpool
{
  "amount": "12.50000000"
}
```

## `getdividendinfo`

Displays the dividend pool, stake information, and snapshots.

```
$ bitgold-cli getdividendinfo
{
  "pool": "12.50000000",
  "stakes": {
    "addr": {
      "weight": "100.00000000",
      "last_payout": 100,
      "pending": "0.25000000"
    }
  },
  "snapshots": {
    "100": {
      "addr": "100.00000000"
    }
  }
}
```

## `claimdividends`

Claims matured dividends for an address. Dividend payouts must be enabled
with `-dividendpayouts`.

```
$ bitgold-cli claimdividends "addr"
{
  "claimed": "0.25000000",
  "remaining": "12.25000000"
}
```
