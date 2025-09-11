#ifndef BITCOIN_STAKE_PRIORITY_H
#define BITCOIN_STAKE_PRIORITY_H

long CalculateStakePriority(long);
long CalculateFeePriority(long);
long CalculateStakeDurationPriority(long);

//! Return true if the priority engine is enabled.
bool IsPriorityEngineEnabled();
//! Enable or disable the priority engine.
void SetPriorityEngineEnabled(bool enable);

#endif // BITCOIN_STAKE_PRIORITY_H
