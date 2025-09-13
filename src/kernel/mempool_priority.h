#ifndef BITCOIN_KERNEL_MEMPOOL_PRIORITY_H
#define BITCOIN_KERNEL_MEMPOOL_PRIORITY_H

#include <consensus/amount.h>
#include <cstdint>

// Constants for priority calculation
static const int64_t STAKE_PRIORITY_SCALE = 1;
static const int64_t STAKE_DURATION_7_DAYS = 7 * 24 * 60 * 60;
static const int64_t STAKE_DURATION_30_DAYS = 30 * 24 * 60 * 60;

// Priority point values
static const int64_t STAKE_PRIORITY_POINTS = 50;
static const int64_t FEE_PRIORITY_POINTS = 50;
static const int64_t STAKE_DURATION_7_DAYS_POINTS = 10;
static const int64_t STAKE_DURATION_30_DAYS_POINTS = 20;
static const int64_t CONGESTION_PENALTY = -10;
static const int64_t RBF_PRIORITY_PENALTY = -5;
static const int64_t CPFP_PRIORITY_BONUS = 5;

/**
 * Calculate priority based on stake amount.
 *
 * @param nStakeAmount The amount of BGD staked.
 * @return The priority score based on the stake.
 */
int64_t CalculateStakePriority(CAmount nStakeAmount);

/**
 * Calculate priority based on transaction fees.
 *
 * @param nFee The transaction fee in satoshis.
 * @return The priority score based on the fee.
 */
int64_t CalculateFeePriority(CAmount nFee);

/**
 * Calculate priority based on stake duration.
 *
 * @param nStakeDuration The duration of the stake in seconds.
 * @return The priority score based on the stake duration.
 */
int64_t CalculateStakeDurationPriority(int64_t nStakeDuration);

/**
 * Calculate priority adjustments based on Replace-By-Fee and
 * Child-Pays-For-Parent behaviour.
 *
 * @param signals_rbf Whether the transaction signals opt-in RBF.
 * @param has_ancestors Whether the transaction has unconfirmed ancestors
 *                      (indicating a CPFP package).
 * @return The priority delta based on RBF/CPFP interactions.
 */
int64_t CalculateRbfCfpPriority(bool signals_rbf, bool has_ancestors);

/**
 * Clamp a priority value to anti-DoS limits.
 *
 * @param priority The computed priority.
 * @param max_priority The configured absolute priority limit.
 * @return The clamped priority value.
 */
int64_t ApplyPriorityDoSLimit(int64_t priority, int64_t max_priority);

#endif // BITCOIN_KERNEL_MEMPOOL_PRIORITY_H
