#include "AmuxProcessor.h"

#include "../../Database/AmuxData.h"

namespace Core::Systems::Flcc
{
AmuxProcessor::AmuxProcessor(
	AmuxData& amux)
	: amux_(amux)
{
}

void AmuxProcessor::step(TaskScheduledTime scheduled_time, double dt_s)
{
	// 尚未實作：AMUX 接收、傳送與資料處理。
	(void)scheduled_time;
	(void)dt_s;
}
}
