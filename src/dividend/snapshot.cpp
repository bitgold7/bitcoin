#include <dividend/snapshot.h>

namespace dividend {

Snapshot CreateSnapshot(const std::map<std::string, StakeInfo>& stakes)
{
    Snapshot snap;
    for (const auto& [addr, info] : stakes) {
        snap.emplace(addr, info.weight);
    }
    return snap;
}

void StoreSnapshot(SnapshotMap& snapshots, const uint256& block_hash, const std::map<std::string, StakeInfo>& stakes)
{
    snapshots.emplace(block_hash, CreateSnapshot(stakes));
}

} // namespace dividend
