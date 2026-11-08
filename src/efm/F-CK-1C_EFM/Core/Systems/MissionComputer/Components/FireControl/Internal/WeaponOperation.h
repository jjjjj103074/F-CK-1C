#pragma once

#include "Targeting.h"
#include "../../../Database/StoresCommandQueue.h"

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
	                Targeting& targeting_control,
	                StoresCommandQueue& stores_commands);
	void handle(const Command& command);
	void stop();
	void step(std::chrono::nanoseconds time, double dt_s);

private:
	void finish_gun();
	void finish_pickle(bool unlock_target);
	const MissionInputs& input_;
	const ModeData& mode_;
	const TargetingData& targeting_;
	WeaponOperationData& weapon_;
	Targeting& targeting_control_;
	StoresCommandQueue& stores_commands_;
	TimedAction gun_;
	TimedAction pickle_;
};
}
