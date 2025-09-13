# Operating Notes

This document summarizes default limits enforced by the node.

- **Block size**: Up to 20&nbsp;MWU (20 million weight units) per block.
- **Signature operations**: Up to 100,000 sigops per block.
- **Mempool size**: Defaults to 500&nbsp;MB for normal operation. When running with `-blocksonly`, the mempool is limited to 8&nbsp;MB.
- **Pruned disk usage**: When running with `-prune` and the default target, syncing roughly 100k blocks used approximately 560&nbsp;MB of disk space in testing.

These limits are reflected in the sample configuration files and operator guidance. See [`doc/bitcoin-conf.md`](bitcoin-conf.md) and the example configs in [`doc/examples`](examples) for details.

## Hardware requirements

Running a full node with the default policy limits typically requires:

- **CPU**: 2 or more cores.
- **Memory**: At least 2&nbsp;GB of RAM.
- **Disk**: Around 1&nbsp;TB of free disk space.
- **Network**: A reliable broadband connection with at least 1&nbsp;MB/s upload speed.

Actual requirements vary with network conditions and optional indexes.
