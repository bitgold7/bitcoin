// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <consensus/merkle.h>
#include <hash.h>
#include <random.h>
#include <uint256.h>

#include <atomic>
#include <cassert>
#include <cstdlib>
#include <thread>
#include <unordered_map>
#include <vector>

/** Build a merkle branch for the leaf at position @p index. */
static std::vector<uint256> BuildMerkleBranch(const std::vector<uint256>& leaves, uint32_t index)
{
    std::vector<uint256> branch;
    std::vector<uint256> current = leaves;
    uint32_t idx = index;
    while (current.size() > 1) {
        if ((idx ^ 1U) < current.size()) {
            branch.push_back(current[idx ^ 1U]);
        } else {
            branch.push_back(current[idx]);
        }
        std::vector<uint256> next;
        next.resize((current.size() + 1) / 2);
        for (size_t i = 0; i < current.size(); i += 2) {
            const uint256& left = current[i];
            const uint256& right = (i + 1 < current.size()) ? current[i + 1] : current[i];
            next[i / 2] = Hash(left, right);
        }
        current = std::move(next);
        idx >>= 1;
    }
    return branch;
}

/** Recompute the merkle root from a branch. */
static uint256 ComputeMerkleRootFromBranch(const uint256& leaf, const std::vector<uint256>& branch, uint32_t index)
{
    uint256 hash = leaf;
    uint32_t idx = index;
    for (const auto& otherside : branch) {
        if (idx & 1) {
            hash = Hash(otherside, hash);
        } else {
            hash = Hash(hash, otherside);
        }
        idx >>= 1;
    }
    return hash;
}

static void MerkleProofGeneration(benchmark::Bench& bench)
{
    constexpr size_t LEAVES = 1024;
    FastRandomContext rng(true);
    std::vector<uint256> leaves(LEAVES);
    for (auto& l : leaves) l = rng.rand256();
    bench.unit("proof").run([&] {
        uint32_t i = rng.randrange(LEAVES);
        auto branch = BuildMerkleBranch(leaves, i);
        benchmark::DoNotOptimize(branch);
    });
}

static void MerkleProofVerificationCached(benchmark::Bench& bench)
{
    constexpr size_t LEAVES = 1024;
    FastRandomContext rng(true);
    std::vector<uint256> leaves(LEAVES);
    for (auto& l : leaves) l = rng.rand256();
    bool mutated = false;
    const uint256 root = ComputeMerkleRoot(std::vector<uint256>(leaves), &mutated);

    std::vector<std::vector<uint256>> branches(LEAVES);
    for (size_t i = 0; i < LEAVES; ++i) branches[i] = BuildMerkleBranch(leaves, i);

    size_t cache_limit = 128;
    if (const char* env = std::getenv("MERKLE_PROOF_CACHE")) {
        size_t val = std::strtoul(env, nullptr, 0);
        if (val > 0) cache_limit = val;
    }

    std::unordered_map<uint32_t, uint256> cache;
    bench.batch(LEAVES).unit("proof").run([&] {
        for (uint32_t i = 0; i < LEAVES; ++i) {
            uint256 hash;
            auto it = cache.find(i);
            if (it != cache.end()) {
                hash = it->second;
            } else {
                hash = ComputeMerkleRootFromBranch(leaves[i], branches[i], i);
                if (cache.size() >= cache_limit) cache.clear();
                cache.emplace(i, hash);
            }
            assert(hash == root);
        }
    });
}

static void MerkleProofVerificationMultiThread(benchmark::Bench& bench)
{
    constexpr size_t LEAVES = 1024;
    FastRandomContext rng(true);
    std::vector<uint256> leaves(LEAVES);
    for (auto& l : leaves) l = rng.rand256();
    bool mutated = false;
    const uint256 root = ComputeMerkleRoot(std::vector<uint256>(leaves), &mutated);

    std::vector<std::vector<uint256>> branches(LEAVES);
    for (size_t i = 0; i < LEAVES; ++i) branches[i] = BuildMerkleBranch(leaves, i);

    unsigned int threads = std::thread::hardware_concurrency();
    if (const char* env = std::getenv("MERKLE_PROOF_THREADS")) {
        unsigned int val = std::strtoul(env, nullptr, 0);
        if (val > 0) threads = val;
    }
    threads = std::max(1u, threads);

    bench.batch(LEAVES).unit("proof").run([&] {
        std::atomic<size_t> offset{0};
        std::vector<std::thread> workers;
        workers.reserve(threads);
        for (unsigned int t = 0; t < threads; ++t) {
            workers.emplace_back([&]() {
                size_t i;
                while ((i = offset.fetch_add(1)) < LEAVES) {
                    uint256 hash = ComputeMerkleRootFromBranch(leaves[i], branches[i], i);
                    assert(hash == root);
                }
            });
        }
        for (auto& th : workers) th.join();
    });
}

BENCHMARK(MerkleProofGeneration, benchmark::PriorityLevel::LOW);
BENCHMARK(MerkleProofVerificationCached, benchmark::PriorityLevel::MEDIUM);
BENCHMARK(MerkleProofVerificationMultiThread, benchmark::PriorityLevel::MEDIUM);
