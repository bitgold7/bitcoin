#ifndef BITCOIN_WALLET_BITGOLDSTAKER_H
#define BITCOIN_WALLET_BITGOLDSTAKER_H

#include <atomic>
#include <thread>
#include <consensus/amount.h>

namespace wallet {

class CWallet;

/** BitGoldStaker runs a background thread performing simple proof-of-stake
 *  block creation by selecting mature UTXOs and submitting new blocks. The
 *  implementation is minimal and intended for experimental use only. */
class BitGoldStaker
{
public:
    explicit BitGoldStaker(CWallet& wallet);
    ~BitGoldStaker();

    /** Start the staking thread. */
    void Start();
    /** Stop the staking thread. */
    void Stop();
    /** Return true if the staking thread is active. */
    bool IsActive() const;

    /** Record a staking attempt. */
    void RecordAttempt();
    /** Record a successful stake with the given reward. */
    void RecordSuccess(CAmount reward);

    struct Stats
    {
        uint64_t attempts{0};
        uint64_t successes{0};
        CAmount rewards{0};
    };

    /** Return current staking statistics. */
    Stats GetStats() const;

private:
    /** Main staking thread loop. Gathers eligible UTXOs, checks stake kernels,
     *  builds coinstake transactions and blocks, and broadcasts them with
     *  basic back-off handling.
     */
    void ThreadStakeMiner();

    CWallet& m_wallet;
    std::thread m_thread;
    std::atomic<bool> m_stop{false};

    std::atomic<uint64_t> m_attempts{0};
    std::atomic<uint64_t> m_successes{0};
    std::atomic<CAmount> m_rewards{0};
};

} // namespace wallet

#endif // BITCOIN_WALLET_BITGOLDSTAKER_H
