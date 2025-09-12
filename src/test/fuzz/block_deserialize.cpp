#include <consensus/validation.h>
#include <primitives/block.h>
#include <streams.h>
#include <test/fuzz/fuzz.h>

FUZZ_TARGET(block_deserialize)
{
    DataStream ds{buffer};
    CBlock block;
    try {
        ds >> TX_WITH_WITNESS(block);
    } catch (const std::ios_base::failure&) {
        return;
    }
    (void)block.GetHash();
    (void)GetBlockWeight(block);
}
