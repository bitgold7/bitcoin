# BitGold Operator Guide

This guide outlines the basic steps for deploying and maintaining BitGold infrastructure.

## Node Setup

1. **Install** the `bitgoldd` daemon and utilities from official releases.
2. **Configure** `bitgold.conf` with your desired network, staking, and RPC settings.
3. **Open Ports** 9888/TCP and 9888/UDP on your firewall to allow peer connections.
4. **Run** `bitgoldd` as a system service and verify it begins synchronizing with the network.

## Staking

Enable staking by setting `staking=1` in `bitgold.conf` and unlocking your wallet for staking with `walletpassphrase <pass> 0 true`. Monitor staking status via `getstakinginfo`.

## Monitoring

Use the `getnetworkinfo`, `getstakinginfo`, and `getblockchaininfo` RPC commands to monitor node health. For production deployments integrate with external monitoring systems to track resource usage and network connectivity.

## Block and Mempool Limits

By default the node enforces a maximum block weight of 20,000,000 weight units and a block-wide budget of 100,000 signature operations. Individual transactions are limited by policy to 400,000 weight units. The functional test suite includes coverage that constructs and mines a full 20&nbsp;M weight block containing a transaction exercising the entire 100&nbsp;k sigop allotment to ensure both consensus and mempool acceptance remain stable, even when `-bgdpriority` is enabled.

## Upgrades

Stay current with new releases. Stop the daemon, install the update, and restart. For minimal downtime consider running redundant nodes and updating them in sequence.

For more advanced configurations consult the documentation in the `doc` directory and the community wiki.
