#include <primitives/transaction.h>
#include <streams.h>
#include <test/fuzz/fuzz.h>

FUZZ_TARGET(tx_deserialize)
{
    DataStream ds{buffer};
    CMutableTransaction mtx;
    try {
        ds >> TX_WITH_WITNESS(mtx);
    } catch (const std::ios_base::failure&) {
        return;
    }
    const CTransaction tx{mtx};
    (void)tx.GetHash();
    (void)tx.GetTotalSize();
}
