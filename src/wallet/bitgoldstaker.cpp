#include <addresstype.h>
#include <chrono>
#include <consensus/amount.h>
#include <consensus/consensus.h>
#include <consensus/merkle.h>
#include <dividend/dividend.h>
#include <interfaces/chain.h>
#include <key.h>
#include <node/context.h>
#include <node/miner.h>
#include <node/stake_modifier_manager.h>
#include <pos/difficulty.h>
#include <pos/stake.h>
#include <script/standard.h>
#include <util/time.h>
#include <validation.h>
#include <wallet/bitgoldstaker.h>
#include <wallet/stake.h>
#include <wallet/spend.h>
#include <wallet/wallet.h>

#include <algorithm>
#include <logging.h>
#include <optional>
#include <vector>

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
    m_thread = std::thread(&BitGoldStaker::ThreadStakeMiner, this);
}

void BitGoldStaker::Stop()
{
    m_stop = true;
    if (m_thread.joinable()) m_thread.join();
}

bool BitGoldStaker::IsActive() const
{
    return m_thread.joinable() && !m_stop;
}

void BitGoldStaker::ThreadStakeMiner()
{
    interfaces::Chain& chain = m_wallet.chain();
    node::NodeContext* node_context = chain.context();
    if (!node_context || !node_context->chainman) {
        LogDebug(BCLog::STAKING, "ThreadStakeMiner: chainman unavailable\n");
        return;
    }
    ChainstateManager& chainman = *node_context->chainman;
    const Consensus::Params& consensus = chainman.GetParams().GetConsensus();
    const CAmount MIN_STAKE_AMOUNT{1 * COIN};
    const int MIN_STAKE_DEPTH{COINBASE_MATURITY};

    std::chrono::milliseconds sleep_time{500};
    while (!m_stop) {
        bool staked{false};
        try {
            int chain_height;
            {
                LOCK(::cs_main);
                chain_height = chainman.ActiveChain().Height();
            }
            const int min_depth = chain_height < MIN_STAKE_DEPTH ? 0 : MIN_STAKE_DEPTH;
            const std::chrono::seconds min_age =
                chain_height < MIN_STAKE_DEPTH ? std::chrono::seconds{0} : std::chrono::seconds{consensus.nStakeMinAge};

            std::vector<COutput> candidates =
                m_wallet.GetStakeableCoins(min_depth, min_age, MIN_STAKE_AMOUNT);

            CAmount total_value{0};
            for (const COutput& o : candidates)
                total_value += o.txout.nValue;
            const CAmount reserve_balance = m_wallet.GetReserveBalance();
            CAmount stakeable_balance{0};
            if (total_value <= reserve_balance) {
                LogDebug(BCLog::STAKING, "ThreadStakeMiner: balance below reserve\n");
                candidates.clear();
            } else {
                stakeable_balance = total_value - reserve_balance;
                candidates.erase(std::remove_if(candidates.begin(), candidates.end(),
                                                [stakeable_balance](const COutput& o) {
                                                    return o.txout.nValue > stakeable_balance;
                                                }),
                                 candidates.end());
                std::sort(candidates.begin(), candidates.end(),
                          [](const COutput& a, const COutput& b) {
                              return a.txout.nValue < b.txout.nValue;
                          });
            }

            if (candidates.empty()) {
                LogDebug(BCLog::STAKING, "ThreadStakeMiner: no eligible UTXOs\n");
            } else {
                CBlockIndex* pindexPrev;
                {
                    LOCK(::cs_main);
                    pindexPrev = chainman.ActiveChain().Tip();
                }
                if (!pindexPrev) {
                    LogDebug(BCLog::STAKING, "ThreadStakeMiner: no tip block\n");
                } else {
                    for (const COutput& stake_out : candidates) {
                        uint256 confirmed_block_hash;
                        {
                            LOCK(m_wallet.cs_wallet);
                            const CWalletTx* wtx = m_wallet.GetWalletTx(stake_out.outpoint.hash);
                            if (!wtx) {
                                LogDebug(BCLog::STAKING, "ThreadStakeMiner: missing wallet tx\n");
                                continue;
                            }
                            auto* conf = wtx->state<TxStateConfirmed>();
                            if (!conf) {
                                LogDebug(BCLog::STAKING, "ThreadStakeMiner: staking tx not confirmed\n");
                                continue;
                            }
                            confirmed_block_hash = conf->confirmed_block_hash;
                        }
                        const CBlockIndex* pindexFrom;
                        {
                            LOCK(::cs_main);
                            pindexFrom = chainman.m_blockman.LookupBlockIndex(confirmed_block_hash);
                        }
                        if (!pindexFrom) {
                            LogDebug(BCLog::STAKING, "ThreadStakeMiner: staking tx block not found\n");
                            continue;
                        }

                        unsigned int nTimeTx{0};
                        if (!GetStakeTime(*pindexPrev, consensus, nTimeTx)) {
                            continue;
                        }
                        unsigned int nBits = GetPoSNextTargetRequired(pindexPrev, nTimeTx, consensus);
                        uint256 hash_proof;
                        LogTrace(BCLog::STAKING, "ThreadStakeMiner: checking kernel for %s", stake_out.outpoint.ToString());
                        RecordAttempt();
                        node::StakeModifierManager& man = *Assert(node_context->stake_modman);
                        if (!CheckStakeKernelHash(pindexPrev, nBits, pindexFrom->GetBlockHash(), pindexFrom->nTime,
                                                  stake_out.txout.nValue, stake_out.outpoint, nTimeTx, man, hash_proof, true,
                                                  consensus)) {
                            LogDebug(BCLog::STAKING, "ThreadStakeMiner: kernel check failed\n");
                            continue;
                        }

                        // Assemble block transactions and calculate fees
                        CTxMemPool* mempool = node_context->mempool.get();
                        BlockAssembler::Options options;
                        ApplyArgsManOptions(gArgs, options);
                        BlockAssembler assembler{chainman.ActiveChainstate(), mempool, options};
                        std::unique_ptr<CBlockTemplate> pblocktemplate = assembler.CreateNewBlock();
                        CAmount fees{0};
                        for (const CAmount& fee : pblocktemplate->vTxFees) {
                            fees += fee;
                        }

                        CMutableTransaction coinstake;
                        coinstake.nLockTime = nTimeTx;
                        coinstake.vin.emplace_back(stake_out.outpoint);
                        coinstake.vin[0].nSequence = CTxIn::SEQUENCE_FINAL;
                        coinstake.vout.resize(3);
                        coinstake.vout[0].SetNull();
                        int64_t coin_age_weight = int64_t(nTimeTx) - int64_t(pindexFrom->GetBlockTime());
                        CAmount subsidy = GetProofOfStakeReward(pindexPrev->nHeight + 1, /*fees=*/0,
                                                                coin_age_weight, consensus);
                        CAmount total_reward = subsidy + fees;
                        CAmount dividend_reward = total_reward / 10;
                        CAmount validator_reward = total_reward - dividend_reward;
                        CAmount input_total = stake_out.txout.nValue;
                        CAmount total = input_total + validator_reward;
                        CAmount split_threshold = 2 * MIN_STAKE_AMOUNT;
                        if (total < split_threshold) {
                            for (const COutput& merge_out : candidates) {
                                if (merge_out.outpoint == stake_out.outpoint) continue;
                                if (input_total + merge_out.txout.nValue > stakeable_balance) break;
                                coinstake.vin.emplace_back(merge_out.outpoint);
                                coinstake.vin.back().nSequence = CTxIn::SEQUENCE_FINAL;
                                input_total += merge_out.txout.nValue;
                                total = input_total + validator_reward;
                                if (total >= split_threshold) break;
                            }
                        }
// Keep vout[0] (coinstake marker) intact, rebuild the rest deterministically.
CScript dividendScript = dividend::GetDividendScript();  // or: CScript() << OP_TRUE

// Ensure we start from a known state: only the marker output at index 0.
if (coinstake.vout.size() > 1) {
    coinstake.vout.resize(1);
}

// Build stake outputs (split if large enough), then append the dividend output.
if (total > split_threshold * 2) {
    const CAmount half = total / 2;
    coinstake.vout.emplace_back(half,            stake_out.txout.scriptPubKey);
    coinstake.vout.emplace_back(total - half,    stake_out.txout.scriptPubKey);
} else {
    coinstake.vout.emplace_back(total,           stake_out.txout.scriptPubKey);
}

// Dividend output last.
coinstake.vout.emplace_back(dividend_reward, dividendScript);

                        {
                            LOCK(m_wallet.cs_wallet);
                            if (!m_wallet.SignTransaction(coinstake)) {
                                LogDebug(BCLog::STAKING, "ThreadStakeMiner: failed to sign coinstake\n");
                                continue;
                            }
                        }

#ifdef ENABLE_BULLETPROOFS
                        std::vector<CBulletproof> bp;
                        if (!CreateBulletproofProof(m_wallet, coinstake, bp)) {
                            LogDebug(BCLog::STAKING, "ThreadStakeMiner: failed to create bulletproof proof\n");
                            continue;
                        }
#endif

                        CMutableTransaction coinbase;
                        coinbase.vin.resize(1);
                        coinbase.vin[0].prevout.SetNull();
                        coinbase.vin[0].nSequence = CTxIn::MAX_SEQUENCE_NONFINAL;
                        coinbase.vin[0].scriptSig = CScript() << (pindexPrev->nHeight + 1) << OP_0;
                        coinbase.vout.resize(1);
                        coinbase.vout[0].nValue = 0;
                        coinbase.nLockTime = pindexPrev->nHeight + 1;

                        CBlock block;
                        block.vtx.emplace_back(MakeTransactionRef(std::move(coinbase)));
                        block.vtx.emplace_back(MakeTransactionRef(std::move(coinstake)));
                        for (size_t i = 1; i < pblocktemplate->block.vtx.size(); ++i) {
                            block.vtx.emplace_back(pblocktemplate->block.vtx[i]);
                        }
                        block.hashPrevBlock = pindexPrev->GetBlockHash();
                        block.nVersion = chainman.m_versionbitscache.ComputeBlockVersion(pindexPrev, consensus);
                        block.nTime = nTimeTx;
                        block.nBits = nBits;
                        block.nNonce = 0;
                        block.hashMerkleRoot = BlockMerkleRoot(block);

                        // Sign the block with the staking key
                        {
                            CTxDestination dest;
                            if (!ExtractDestination(stake_out.txout.scriptPubKey, dest)) {
                                LogDebug(BCLog::STAKING, "ThreadStakeMiner: failed to extract destination for signing\n");
                                continue;
                            }
                            const PKHash* keyhash = std::get_if<PKHash>(&dest);
                            if (!keyhash) {
                                LogDebug(BCLog::STAKING, "ThreadStakeMiner: unsupported script for signing\n");
                                continue;
                            }
                            std::optional<CKey> key = m_wallet.GetKey(ToKeyID(*keyhash));
                            if (!key || !key->Sign(block.GetHash(), block.vchBlockSig)) {
                                LogDebug(BCLog::STAKING, "ThreadStakeMiner: failed to sign block\n");
                                continue;
                            }
                        }

                        {
                            LOCK(cs_main);
                            if (!ContextualCheckProofOfStake(block, pindexPrev,
                                                             chainman.ActiveChainstate().CoinsTip(),
                                                             chainman.ActiveChain(), consensus)) {
                                LogDebug(BCLog::STAKING, "ThreadStakeMiner: produced block failed CheckProofOfStake\n");
                                continue;
                            }
                        }

                        bool new_block{false};
                        if (!chainman.ProcessNewBlock(std::make_shared<const CBlock>(block),
                                                      /*force_processing=*/true,
                                                      &new_block)) {
                            LogDebug(BCLog::STAKING, "ThreadStakeMiner: ProcessNewBlock failed\n");
                            continue;
                        }
                        LogPrintLevel(BCLog::STAKING, BCLog::Level::Info,
                                      "ThreadStakeMiner: staked block %s\n",
                                      block.GetHash().ToString());
                        RecordSuccess(validator_reward);
                        m_wallet.AddStakingReward(validator_reward);
                        staked = true;
                        break;
                    }
                }
            }
        } catch (const std::exception& e) {
            LogDebug(BCLog::STAKING, "ThreadStakeMiner exception: %s\n", e.what());
        }

        if (staked) {
            sleep_time = std::chrono::milliseconds{500};
        } else {
            sleep_time = std::min(sleep_time * 2, std::chrono::milliseconds{8000});
        }
        std::this_thread::sleep_for(sleep_time);
    }
}

void BitGoldStaker::RecordAttempt()
{
    ++m_attempts;
}

void BitGoldStaker::RecordSuccess(CAmount reward)
{
    ++m_successes;
    m_rewards += reward;
}

BitGoldStaker::Stats BitGoldStaker::GetStats() const
{
    return Stats{m_attempts.load(), m_successes.load(), m_rewards.load()};
}

} // namespace wallet
