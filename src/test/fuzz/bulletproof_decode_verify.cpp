#include <bulletproofs.h>
#include <streams.h>
#include <test/fuzz/fuzz.h>

FUZZ_TARGET(bulletproof_decode_verify)
{
#ifdef ENABLE_BULLETPROOFS
    DataStream ds{buffer};
    CBulletproof bp;
    try {
        ds >> bp;
    } catch (const std::ios_base::failure&) {
        return;
    }
    (void)VerifyBulletproof(bp);
#else
    (void)buffer;
#endif
}
