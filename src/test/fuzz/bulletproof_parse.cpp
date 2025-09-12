#include <bulletproofs.h>
#include <script/script.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>

FUZZ_TARGET(bulletproof_parse)
{
    FuzzedDataProvider fdp{buffer.data(), buffer.size()};
#ifdef ENABLE_BULLETPROOFS
    CBulletproof bp;
    const auto commit = fdp.ConsumeBytes<unsigned char>(33);
    if (commit.size() == 33) {
        std::memcpy(bp.commitment.data, commit.data(), 33);
    }
    const size_t proof_size = fdp.ConsumeIntegralInRange<size_t>(0, 2 * SECP256K1_RANGE_PROOF_MAX_LENGTH);
    bp.proof = fdp.ConsumeBytes<unsigned char>(proof_size);
    const size_t extra_size = fdp.ConsumeIntegralInRange<size_t>(0, 1024);
    bp.extra = fdp.ConsumeBytes<unsigned char>(extra_size);

    DataStream ds{};
    try {
        ds << bp;
        CBulletproof decoded;
        ds >> decoded;
        (void)VerifyBulletproof(decoded);
    } catch (const std::ios_base::failure&) {
        return;
    }

    const auto script_bytes = fdp.ConsumeRandomLengthByteVector(256);
    CScript script(script_bytes.begin(), script_bytes.end());
    CBulletproof extracted;
    bool malformed{false};
    CScript::const_iterator pc{script.begin()};
    opcodetype opcode;
    std::vector<unsigned char> data;
    if (script.GetOp(pc, opcode) && opcode == OP_BULLETPROOF) {
        if (!script.GetOp(pc, opcode, data) || data.size() != sizeof(extracted.commitment.data)) {
            malformed = true;
        } else {
            std::memcpy(extracted.commitment.data, data.data(), sizeof(extracted.commitment.data));
            if (!script.GetOp(pc, opcode, extracted.proof)) {
                malformed = true;
            }
        }
        if (!malformed) {
            (void)VerifyBulletproof(extracted);
        }
    }
#else
    (void)fdp;
#endif
}
