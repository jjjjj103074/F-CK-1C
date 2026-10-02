#include "StartupAndRestart.h"

#include "../../Database/ControlLawData.h"
#include "../../Database/SystemStatusTables.h"

namespace Core::Systems::Flcc
{
StartupAndRestart::StartupAndRestart(
	ControlLawData& control_law,
	SystemStatusTables& system_status)
	: control_law_(control_law),
	system_status_(system_status)
{
}

void StartupAndRestart::step(std::chrono::nanoseconds scheduled_time, double dt_s)
{
	// 尚未實作：啟動檢查、資料初始化與重新啟動程序。
	(void)scheduled_time;
	(void)dt_s;
}
}
