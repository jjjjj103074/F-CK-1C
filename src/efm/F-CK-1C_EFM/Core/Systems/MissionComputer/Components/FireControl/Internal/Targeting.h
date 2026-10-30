#pragma once

#include "../../TimedAction.h"
#include "../../../Database/InputOutputData.h"
#include "../../../Database/MissionData.h"
#include "../../StoresManagement/StoresManagement.h"

namespace Core::Systems::Mc::Internal
{
/// @brief 火控內部的目標操作與捕獲要求；雷達觀測和實際處理由外部雷達模型提供。
class Targeting final
{
public:
	Targeting(const MissionInputs& input,
	          const ModeData& mode,
	          const StoresData& stores,
	          TargetingData& targeting,
	          ExternalActionQueue& actions,
	          StoresManagement& stores_management);
	void handle(const Command& command);
	void select_mode(bool cycle_weapon);
	void prepare_release();
	void unlock();
	void step(std::chrono::nanoseconds time, double dt_s);

private:
	void ensure_radar_on();
	void apply_submode(AirToAirSubmode mode);
	void begin_lock(bool pulse);
	void finish_lock();
	const MissionInputs& input_;
	const ModeData& mode_;
	const StoresData& stores_;
	TargetingData& targeting_;
	ExternalActionQueue& actions_;
	StoresManagement& stores_management_;
	TimedAction lock_;
	bool radar_on_requested_ = false;
	std::chrono::nanoseconds next_auto_lock_ = {};
};
}
