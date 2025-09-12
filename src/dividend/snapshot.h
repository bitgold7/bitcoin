#ifndef BITCOIN_DIVIDEND_SNAPSHOT_H
#define BITCOIN_DIVIDEND_SNAPSHOT_H

#include <dividend/dividend.h>
#include <map>
#include <string>
#include <uint256.h>

namespace dividend {

using Snapshot = std::map<std::string, CAmount>;
using SnapshotMap = std::map<uint256, Snapshot>;

Snapshot CreateSnapshot(const std::map<std::string, StakeInfo>& stakes);
void StoreSnapshot(SnapshotMap& snapshots, const uint256& block_hash, const std::map<std::string, StakeInfo>& stakes);

} // namespace dividend

#endif // BITCOIN_DIVIDEND_SNAPSHOT_H
