#ifndef BITCOIN_BULLETPROOFS_UTILS_H
#define BITCOIN_BULLETPROOFS_UTILS_H

// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <array>
#include <vector>

#include <bulletproofs.h>
#include <hash.h>
#include <primitives/transaction.h>
#include <serialize.h>
#include <span.h>

inline std::vector<std::vector<unsigned char>> ComputeBulletproofCommitments(const CTransaction& tx)
{
    std::vector<std::vector<unsigned char>> commitments;
    commitments.reserve(tx.vout.size());
    for (size_t idx = 0; idx < tx.vout.size(); ++idx) {
        const CTxOut& txout = tx.vout[idx];

        CHash256 commitment_hasher;
        commitment_hasher.Write(MakeUCharSpan(tx.GetHash()));

        std::array<unsigned char, sizeof(uint32_t)> index_bytes{};
        WriteLE32(index_bytes.data(), static_cast<uint32_t>(idx));
        commitment_hasher.Write(index_bytes);

        std::array<unsigned char, sizeof(uint64_t)> value_bytes{};
        WriteLE64(value_bytes.data(), static_cast<uint64_t>(txout.nValue));
        commitment_hasher.Write(value_bytes);

        commitment_hasher.Write(MakeUCharSpan(txout.scriptPubKey));

        std::vector<unsigned char> commitment(CHash256::OUTPUT_SIZE);
        commitment_hasher.Finalize(commitment);
        commitments.push_back(std::move(commitment));
    }
    return commitments;
}

inline void PopulateBulletproofProof(const CTransaction& tx, CBulletproof& proof)
{
    proof.commitments = ComputeBulletproofCommitments(tx);
    if (proof.commitments.empty()) {
        proof.proof.clear();
        return;
    }

    CHash256 aggregate;
    for (const auto& commitment : proof.commitments) {
        aggregate.Write(commitment);
    }

    std::array<unsigned char, CHash256::OUTPUT_SIZE> digest{};
    aggregate.Finalize(digest);
    proof.proof.assign(digest.begin(), digest.end());
}

#endif // BITCOIN_BULLETPROOFS_UTILS_H
