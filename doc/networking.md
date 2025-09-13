# Networking Guide

This document summarizes network parameters for the various BitGold networks.

| Network  | Magic bytes | Default port | DNS seed nodes |
|----------|-------------|--------------|----------------|
| Mainnet  | `fcc1c6dc`  | `8888`       | `dnsseed.bitgold.org`, `dnsseed.bitgold.com`, `dnsseed.bitgold.io`, `dnsseed.bitgold.net`, `dnsseed.bitgold.co` |
| Testnet  | `b2d3f4a5`  | `28889`      | `testnet-seed.bitgold.org`, `seed-testnet.bitgold.org` |
| Signet   | `b3c4d5e6`  | `38888`      | *(none)* |
| Regtest  | `abbccdde`  | `38333`      | *(none)* |

Magic bytes appear in the P2P message header and help nodes identify the network
of incoming connections. Default ports indicate where nodes listen for inbound
P2P connections. DNS seeds provide initial peer discovery for new nodes.
