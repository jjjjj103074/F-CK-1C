#include "FailureManager.h"

#include "../../Database/SystemStatusTables.h"

namespace Core::Systems::Flcc
{
FailureManager::FailureManager(
	SystemStatusTables& system_status)
	: system_status_(system_status)
{
}

void FailureManager::step(TaskScheduledTime scheduled_time, double dt_s)
{
	// 尚未實作：故障判定、故障處理與降級策略；本階段不處理備援。
	(void)scheduled_time;
	(void)dt_s;
}
}
