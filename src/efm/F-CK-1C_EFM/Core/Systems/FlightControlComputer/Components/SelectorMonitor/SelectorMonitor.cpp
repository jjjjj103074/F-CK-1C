#include "SelectorMonitor.h"

#include "../../Database/InputOutputData.h"
#include "../../Database/SystemStatusTables.h"

namespace Core::Systems::Flcc
{
SelectorMonitor::SelectorMonitor(
	const InputOutputData& input_output,
	SystemStatusTables& system_status)
	: input_output_(input_output),
	system_status_(system_status)
{
}

void SelectorMonitor::step(TaskScheduledTime scheduled_time, double dt_s)
{
	// 尚未實作：輸入選擇、選擇有效性判斷與狀態更新。
	(void)scheduled_time;
	(void)dt_s;
}
}
