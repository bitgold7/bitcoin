#ifndef BITCOIN_CONSENSUS_DIVIDENDS_SCHEDULE_H
#define BITCOIN_CONSENSUS_DIVIDENDS_SCHEDULE_H

#include <cstdint>
#include <map>
#include <uint256.h>

namespace consensus {
namespace dividends {

//! Interval between dividend snapshots in blocks.
inline constexpr int SNAPSHOT_INTERVAL{16200};

//! Determine if the given height is a snapshot boundary.
inline bool IsSnapshotHeight(int height) { return height > 0 && height % SNAPSHOT_INTERVAL == 0; }

//! Calculate the next snapshot height strictly greater than the provided one.
inline int NextSnapshotHeight(int height) { return ((height / SNAPSHOT_INTERVAL) + 1) * SNAPSHOT_INTERVAL; }

//!
//! Track whether a snapshot should run at the provided height.
//! Returns true if this is the first time this height is seen and records the
//! block hash to avoid paying twice across reorgs.
//!
inline bool ScheduleSnapshot(int height, const uint256& hash,
                             std::map<int, uint256>& paid_snapshots)
{
    if (!IsSnapshotHeight(height)) return false;
    auto [it, inserted] = paid_snapshots.emplace(height, hash);
    return inserted;
}

} // namespace dividends
} // namespace consensus

#endif // BITCOIN_CONSENSUS_DIVIDENDS_SCHEDULE_H
