# Mempool priority points

Transactions in the mempool can be assigned **priority points** that
influence their treatment compared to other transactions. The ordering is
determined by `CompareTxByPoints`:

1. Transactions are sorted by priority (higher values first).
2. When priorities are equal, the modified fee rate is used to break ties so
   that fee deltas applied by policy are respected.
3. If both priority and fee rate are equal, the transaction hash provides a
   final deterministic tie-breaker.

Eviction via `TrimToSize()` uses the `descendant_score` index, which sorts
transactions by increasing priority. This ensures that lower-priority entries
are removed before higher-priority ones when the mempool is full.

For block construction the `ancestor_score` index is used, ordering entries by
decreasing priority. As a result miners will consider higher priority
transactions first even if they pay a lower fee rate.

