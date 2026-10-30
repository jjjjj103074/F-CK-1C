#pragma once

#include "Targeting.h"
#include "../../StoresManagement/StoresManagement.h"

namespace Core::Systems::Mc::Internal
{
/// @brief 火控內部的射擊／投放資格與程序；實際 DCS 操作由外部介面執行。
class WeaponOperation final
{
public:
	WeaponOperation(const MissionInputs& input,
	                const ModeData& mode,
	                const TargetingData& targeting,
	                WeaponOperationData& weapon,
	                ExternalActionQueue& actions,
	                Targeting& targeting_control,
	                StoresManagement& stores);
	void handle(const Command& command);
	void stop();
	void step(std::chrono::nanoseconds time, double dt_s);

private:
	void finish_gun();
	void finish_pickle(bool consume);
	const MissionInputs& input_;
	const ModeData& mode_;
	const TargetingData& targeting_;
	WeaponOperationData& weapon_;
	ExternalActionQueue& actions_;
	Targeting& targeting_control_;
	StoresManagement& stores_;
	TimedAction gun_;
	TimedAction pickle_;
};
}
