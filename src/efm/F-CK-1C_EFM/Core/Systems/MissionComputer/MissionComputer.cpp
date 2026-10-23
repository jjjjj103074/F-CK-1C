#include "MissionComputer.h"

#include "../SystemPipeline.h"
#include "../SystemUpdateRates.h"

namespace Core::Systems
{
MissionComputer::MissionComputer()
    : system_monitor_(input_output_data_, system_status_tables_.health),
      failure_manager_(system_status_tables_.health, system_status_tables_.failure_response),
      executive_(executive_tables_, system_monitor_, failure_manager_),
      startup_and_restart_(executive_, system_status_tables_.lifecycle)
{
	startup_and_restart_.start();
}

void MissionComputer::set_powered(bool powered) noexcept
{
	powered_ = powered;
}

bool MissionComputer::powered() const noexcept
{
	return powered_;
}

void MissionComputer::setup(SystemSetup& setup)
{
	// 暫用專案 fallback；不借用 F-16XL 飛控參考值作為 MC 實機依據。
	setup.update_rate_hz(kProjectDefinedFallbackUpdateRateHz);
}

void MissionComputer::step(const SystemStepContext& context, const AircraftDataView& aircraft, SystemResult& result)
{
	if (!powered_)
	{
		return;
	}

	read_inputs(aircraft);
	executive_.step(context.scheduled_time);
	publish_outputs(result);
}

void MissionComputer::read_inputs(const AircraftDataView& aircraft)
{
	// 尚未定義任務輸入；加入欄位時須同步在 setup 宣告讀取。
	(void)aircraft;
}

void MissionComputer::publish_outputs(SystemResult& result) const
{
	// 尚未定義任務輸出；目前不發布 MC 可用或功能已就緒等狀態。
	(void)result;
}
}
