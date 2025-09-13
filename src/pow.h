#ifndef BITCOIN_POW_H
#define BITCOIN_POW_H

#include <cstdint>

class CBlockIndex;
struct CBlockHeader;
namespace Consensus { struct Params; }

/**
 * Return the next proof-of-stake target using an 8 minute retarget rule.
 */
unsigned int GetPoSNextWorkRequired(const CBlockIndex* pindexLast, int64_t nBlockTime, const Consensus::Params& params);

#endif // BITCOIN_POW_H
