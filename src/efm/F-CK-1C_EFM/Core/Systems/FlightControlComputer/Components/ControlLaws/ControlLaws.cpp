#include "ControlLaws.h"

#include "../../Database/InputOutputData.h"
#include "../../Database/ControlLawData.h"
#include "../../Database/SystemStatusTables.h"

namespace Core::Systems::Flcc
{
ControlLaws::ControlLaws(
	InputOutputData& input_output,
	ControlLawData& control_law,
	const SystemStatusTables& system_status)
	: input_output_(input_output),
	control_law_(control_law),
	system_status_(system_status)
{
}

void ControlLaws::step(TaskScheduledTime scheduled_time, double dt_s)
{
	// 尚未實作：控制律、模式、濾波、積分與控制需求計算。
	(void)scheduled_time;
	(void)dt_s;
}
}
