// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <validation.h>

bool ChainstateManager::IsInitialBlockDownload() const
{
    return const_cast<ChainstateManager*>(this)->ActiveChainstate().IsInitialBlockDownload();
}
