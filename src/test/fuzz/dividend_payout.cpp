#include <consensus/dividends/dividend.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>

#include <cassert>
#include <map>
#include <string>

FUZZ_TARGET(dividend_payout)
{
    FuzzedDataProvider fdp(buffer.data(), buffer.size());
    std::map<std::string, StakeInfo> stakes;
    const size_t count = fdp.ConsumeIntegralInRange<size_t>(0, 16);
    for (size_t i = 0; i < count; ++i) {
        std::string addr = fdp.ConsumeBytesAsString(fdp.ConsumeIntegralInRange<size_t>(0, 32));
        StakeInfo info;
        info.weight = fdp.ConsumeIntegral<CAmount>();
        info.last_payout_height = fdp.ConsumeIntegral<int>();
        stakes.emplace(std::move(addr), info);
    }
    int height = fdp.ConsumeIntegral<int>();
    CAmount pool = fdp.ConsumeIntegral<CAmount>();
    auto payouts = dividend::CalculatePayouts(stakes, height, pool);
    for (const auto& [addr, amt] : payouts) {
        assert(amt > 0);
    }
}
