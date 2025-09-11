# Networking Guide

This document summarizes network parameters for the various BitGold networks.

| Network  | Magic bytes | Default port | DNS seed nodes |
|----------|-------------|--------------|----------------|
| Mainnet  | `fbc0c5db`  | `8888`       | `seed.bitgold.org`, `seed.bitgold.net`, `seed.bitgold.co`, `seed.bitgold.io`, `seed.bitgold.info` |
| Testnet  | `b1d2f3a4`  | `28889`      | `testnet-seed.bitgold.org`, `seed-testnet.bitgold.org` |
| Signet   | `b2c3d4e5`  | `38888`      | *(none)* |
| Regtest  | `aabbccdd`  | `38333`      | *(none)* |

Magic bytes appear in the P2P message header and help nodes identify the network
of incoming connections. Default ports indicate where nodes listen for inbound
P2P connections. DNS seeds provide initial peer discovery for new nodes.
