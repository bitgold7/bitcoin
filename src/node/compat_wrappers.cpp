// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <validation.h>
#include <chain.h>
#include <node/blockstorage.h>
#include <memory>

// Forward declaration of the upstream free function
bool ProcessNewBlock(ChainstateManager& chainman, const std::shared_ptr<const CBlock>& block,
                     bool force_processing, bool min_pow_checked, bool* new_block);

bool ChainstateManager::ProcessNewBlock(const std::shared_ptr<const CBlock>& block,
                                        bool force_processing,
                                        bool min_pow_checked,
                                        bool* new_block)
{
    return ::ProcessNewBlock(*this, block, force_processing, min_pow_checked, new_block);
}
