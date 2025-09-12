#include <consensus/pos/stake.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

FUZZ_TARGET(coinstake_parse)
{
    FuzzedDataProvider fdp(buffer.data(), buffer.size());
    DataStream ds{buffer};
    CMutableTransaction mtx;
    try {
        ds >> TX_WITH_WITNESS(mtx);
    } catch (const std::ios_base::failure&) {
        return;
    }
    CTransaction tx{mtx};
    CBlock block;
    block.vtx.emplace_back(MakeTransactionRef(CTransaction()));
    block.vtx.emplace_back(MakeTransactionRef(tx));
    if (fdp.ConsumeBool()) {
        block.vchBlockSig = fdp.ConsumeBytes<unsigned char>(fdp.ConsumeIntegralInRange<size_t>(0, 128));
    }
    (void)IsProofOfStake(block);
    (void)CheckBlockSignature(block);
}
