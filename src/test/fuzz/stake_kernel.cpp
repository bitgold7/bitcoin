#include <consensus/pos/stake.h>
#include <node/stake_modifier_manager.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <primitives/transaction.h>
#include <chain.h>
#include <consensus/params.h>

FUZZ_TARGET(stake_kernel)
{
    FuzzedDataProvider fdp(buffer.data(), buffer.size());
    CBlockIndex dummy{};
    dummy.nHeight = fdp.ConsumeIntegral<unsigned int>();
    dummy.nTime = fdp.ConsumeIntegral<unsigned int>();
    unsigned int nBits = fdp.ConsumeIntegral<unsigned int>();
    uint256 hashBlockFrom = ConsumeUInt256(fdp);
    unsigned int nTimeBlockFrom = fdp.ConsumeIntegral<unsigned int>();
    CAmount amount = fdp.ConsumeIntegral<CAmount>();
    COutPoint prevout{ConsumeUInt256(fdp), fdp.ConsumeIntegral<unsigned int>()};
    unsigned int nTimeTx = fdp.ConsumeIntegral<unsigned int>();
    uint256 hashProofOfStake;
    Consensus::Params params;
    params.nStakeTimestampMask = fdp.ConsumeIntegral<unsigned int>();
    params.nStakeModifierVersion = fdp.ConsumeIntegral<int>();
    node::StakeModifierManager man;
    (void)CheckStakeKernelHash(&dummy, nBits, hashBlockFrom, nTimeBlockFrom, amount, prevout, nTimeTx, man, hashProofOfStake, false, params);
}
