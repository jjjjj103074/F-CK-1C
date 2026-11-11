#pragma once

#include "../../../Database/MissionData.h"
#include "../../../../../Contracts/Commands.h"
#include "../../../Database/StoresCommandQueue.h"

namespace Core::Systems::Mc::Internal
{
/// @brief 武器使用更新的程序結果，由 FC 安排目標接續。
enum class WeaponOperationEvent
{
	None,
	ReleaseInputEnded
};

/// @brief 判斷武器使用資格，提出開始／結束意圖並整理執行結果。
class WeaponOperation final
{
public:
	WeaponOperation(const ModeData& mode,
	                const WeaponUseData& execution,
	                WeaponOperationData& weapon,
	                StoresCommandQueue& stores_commands);
	void handle(const Command& command);
	void stop();
	WeaponOperationEvent step();

private:
	bool can_use_gun() const;
	bool can_release_weapon() const;
	void handle_gun(bool pressed);
	void handle_release(bool pressed);
	const ModeData& mode_;
	const WeaponUseData& execution_;
	WeaponOperationData& weapon_;
	StoresCommandQueue& stores_commands_;
	std::uint64_t reported_release_end_ = 0; ///< 最近已通知 FC 的正常投放結束要求。
};
}
