#pragma once

#include "../../TimedAction.h"
#include "../../../Database/InputOutputData.h"
#include "../../../Database/MissionData.h"
#include "../../../Database/StoresCommandQueue.h"

namespace Core::Systems::Mc::Internal
{
/// @brief 武器操作更新產生的程序結果，由 FC 安排目標接續。
enum class WeaponOperationEvent
{
	None,
	ReleaseInputEnded
};

/// @brief 火控內部的射擊／投放資格與程序；實際 DCS 操作由外部介面執行。
class WeaponOperation final
{
public:
	WeaponOperation(const MissionInputs& input,
	                const ModeData& mode,
	                WeaponOperationData& weapon,
	                StoresCommandQueue& stores_commands);
	void handle(const Command& command);
	void stop();
	WeaponOperationEvent step(std::chrono::nanoseconds time, double dt_s);

private:
	void finish_gun();
	void finish_pickle();
	const MissionInputs& input_;
	const ModeData& mode_;
	WeaponOperationData& weapon_;
	StoresCommandQueue& stores_commands_;
	TimedAction gun_;
	TimedAction pickle_;
};
}
