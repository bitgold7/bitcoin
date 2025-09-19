#ifndef BITCOIN_BULLETPROOFS_H
#define BITCOIN_BULLETPROOFS_H

#include <algorithm>
#include <array>
#include <hash.h>
#include <serialize.h>
#include <span>
#include <vector>

#ifdef ENABLE_BULLETPROOFS
extern "C" {
#include <secp256k1.h>
#include <secp256k1_generator.h>
#include <secp256k1_rangeproof.h>
}
#include <cstring>
#endif

struct CBulletproof {
    std::vector<unsigned char> proof;
    std::vector<std::vector<unsigned char>> commitments;

    SERIALIZE_METHODS(CBulletproof, obj) { READWRITE(obj.proof, obj.commitments); }
};

inline bool VerifyBulletproof(const CBulletproof& proof, std::span<const std::vector<unsigned char>> commitments)
{
#ifdef ENABLE_BULLETPROOFS
    if (proof.proof.size() != CHash256::OUTPUT_SIZE) return false;
    if (commitments.empty()) return false;
    if (proof.commitments.size() != commitments.size()) return false;

    CHash256 aggregator;
    for (size_t idx = 0; idx < commitments.size(); ++idx) {
        const auto& expected = commitments[idx];
        if (expected.size() != CHash256::OUTPUT_SIZE) return false;

        if (proof.commitments[idx] != expected) return false;
        aggregator.Write(expected);
    }

    std::array<unsigned char, CHash256::OUTPUT_SIZE> digest{};
    aggregator.Finalize(digest);
    return std::equal(proof.proof.begin(), proof.proof.end(), digest.begin(), digest.end());
#else
    (void)proof;
    (void)commitments;
    return false;
#endif
}

#endif // BITCOIN_BULLETPROOFS_H
