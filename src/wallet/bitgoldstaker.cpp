#include <wallet/bitgoldstaker.h>
#include <wallet/wallet.h>
#include <wallet/spend.h>
#include <wallet/transaction.h>
#include <interfaces/chain.h>
#include <logging.h>
#include <chrono>
#include <chain.h>
#include <pos/stake.h>
#include <util/time.h>

// The staking helpers currently live in the global namespace. Create an alias
// so the code can refer to them through the pos:: namespace which is the
// intended final location.
namespace pos {
using ::CheckStakeKernelHash;
} // namespace pos

namespace wallet {

BitGoldStaker::BitGoldStaker(CWallet& wallet) : m_wallet(wallet) {}

BitGoldStaker::~BitGoldStaker()
{
    Stop();
}

void BitGoldStaker::Start()
{
    if (m_thread.joinable()) return;
    m_stop = false;
    m_thread = std::thread(&BitGoldStaker::ThreadStaker, this);
}

void BitGoldStaker::Stop()
{
    m_stop = true;
    if (m_thread.joinable()) m_thread.join();
}

void BitGoldStaker::ThreadStaker()
{
    util::ThreadRename("bitgold-staker");

    while (!m_stop) {
        interfaces::Chain& chain = m_wallet.chain();

        // Retrieve the current chain tip information to construct a minimal
        // CBlockIndex used by the kernel hash check.
        CBlock tip_block;
        CBlockIndex pindex_prev(tip_block);
        unsigned int n_bits{0};
        if (auto height = chain.getHeight()) {
            uint256 tip_hash = chain.getBlockHash(*height);
            interfaces::FoundBlock fb;
            fb.data(tip_block).height(*height);
            if (chain.findBlock(tip_hash, fb) && fb.found) {
                pindex_prev.phashBlock = &tip_hash;
                pindex_prev.nHeight = *height;
                pindex_prev.nTime = tip_block.nTime;
                pindex_prev.nBits = tip_block.nBits;
                n_bits = tip_block.nBits;
            }
        }

        // Bail out early if we could not determine the tip information.
        if (pindex_prev.phashBlock == nullptr) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        // Gather wallet coins available for staking.
        std::vector<COutput> coins;
        {
            LOCK(m_wallet.cs_wallet);
            coins = AvailableCoinsListUnspent(m_wallet).All();
        }

        for (const COutput& out : coins) {
            if (m_stop) break;

            // Only mature spendable coins can be staked.
            if (!out.spendable || out.depth <= 0) continue;

            const CWalletTx* wtx{nullptr};
            {
                LOCK(m_wallet.cs_wallet);
                wtx = m_wallet.GetWalletTx(out.outpoint.hash);
            }
            if (!wtx) continue;

            const TxStateConfirmed* confirmed = wtx->state<TxStateConfirmed>();
            if (!confirmed) continue; // require confirmed transactions

            // Load the block that contains the staking input.
            CBlock block_from;
            interfaces::FoundBlock fb_prev;
            fb_prev.data(block_from);
            if (!chain.findBlock(confirmed->confirmed_block_hash, fb_prev) || !fb_prev.found) {
                continue;
            }

            uint256 hash_proof;
            if (!pos::CheckStakeKernelHash(&pindex_prev, n_bits, block_from,
                                           confirmed->position_in_block, wtx->tx,
                                           out.outpoint, static_cast<unsigned int>(GetTime()),
                                           hash_proof, false)) {
                continue;
            }

            // Build the coinstake transaction. The first output is required to be
            // empty. The second pays back to the same script for simplicity.
            CMutableTransaction mtx;
            mtx.nTime = static_cast<unsigned int>(GetTime());
            mtx.vin.emplace_back(out.outpoint);
            mtx.vout.emplace_back();
            mtx.vout.emplace_back(out.txout.nValue, out.txout.scriptPubKey);

            bool signed_tx{false};
            {
                LOCK(m_wallet.cs_wallet);
                signed_tx = m_wallet.SignTransaction(mtx);
            }
            if (!signed_tx) {
                continue;
            }

            // Broadcast the transaction through the wallet interface.
            m_wallet.CommitTransaction(MakeTransactionRef(mtx), {}, {});
            break; // stake once per loop
        }

        // Sleep a bit before the next staking attempt.
        if (!m_stop) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

} // namespace wallet
