#ifndef BITCOIN_VALIDATION_METRICS_H
#define BITCOIN_VALIDATION_METRICS_H

#include <atomic>
#include <cstdint>

struct ValidationMetrics {
    std::atomic<uint64_t> sigcheck_time{0};
    std::atomic<uint64_t> bulletproof_time{0};
    std::atomic<uint64_t> utxo_fetch_time{0};
    std::atomic<uint64_t> script_eval_time{0};

    void Reset() {
        sigcheck_time.store(0, std::memory_order_relaxed);
        bulletproof_time.store(0, std::memory_order_relaxed);
        utxo_fetch_time.store(0, std::memory_order_relaxed);
        script_eval_time.store(0, std::memory_order_relaxed);
    }
};

extern ValidationMetrics g_validation_metrics;

void LogValidationMetrics();

#endif // BITCOIN_VALIDATION_METRICS_H
