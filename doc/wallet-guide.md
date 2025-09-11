# BitGold Wallet Guide

This guide describes how to create and use a BitGold wallet for everyday transactions and staking.

## Creating a Wallet

1. Launch `bitgold-qt` or run `bitgold-cli createwallet <name>`.
2. Back up the generated seed or wallet file to a secure location.

## Sending and Receiving

- Use the **Receive** tab or `getnewaddress` to obtain a BGD address.
- Send coins with the **Send** tab or `sendtoaddress <addr> <amount>`.
- Review activity with the **Transactions** tab or `listtransactions`.

## Staking From the Wallet

Unlock the wallet for staking only using `walletpassphrase <pass> 0 true`. The wallet will automatically participate in PoS once synchronized and funded.

## Dividend Claims

Dividends mature into claimable outputs. Use `claimdividends` (GUI: **Claim** button) to sweep matured dividends into your balance.

## Security Best Practices

- Encrypt your wallet with a strong passphrase.
- Keep backups offline.
- Verify download signatures before installing software updates.

For more detailed usage examples refer to the command line help (`bitgold-cli help`) and the community wiki.
