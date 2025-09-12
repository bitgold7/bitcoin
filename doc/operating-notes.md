# Operating Notes

This document summarizes default limits enforced by the node.

- **Block size**: Up to 20&nbsp;MWU (20 million weight units) per block.
- **Signature operations**: Up to 100,000 sigops per block.
- **Mempool size**: Defaults to 500&nbsp;MB for normal operation. When running with `-blocksonly`, the mempool is limited to 8&nbsp;MB.
- **Pruned disk usage**: When running with `-prune` and the default target, syncing roughly 100k blocks used approximately 560&nbsp;MB of disk space in testing.

These limits are reflected in the sample configuration files and operator guidance. See [`doc/bitcoin-conf.md`](bitcoin-conf.md) and the example configs in [`doc/examples`](examples) for details.
