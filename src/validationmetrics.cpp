#include <validationmetrics.h>
#include <logging.h>

ValidationMetrics g_validation_metrics;

void LogValidationMetrics()
{
    LogPrintLevel(BCLog::BENCH, BCLog::Level::Info,
                  "validation metrics: sigcheck=%uµs bulletproof=%uµs utxo=%uµs script=%uµs\n",
                  (unsigned)g_validation_metrics.sigcheck_time.load(std::memory_order_relaxed),
                  (unsigned)g_validation_metrics.bulletproof_time.load(std::memory_order_relaxed),
                  (unsigned)g_validation_metrics.utxo_fetch_time.load(std::memory_order_relaxed),
                  (unsigned)g_validation_metrics.script_eval_time.load(std::memory_order_relaxed));
}
