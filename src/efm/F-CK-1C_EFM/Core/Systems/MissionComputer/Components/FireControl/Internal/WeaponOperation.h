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
	void handle(const Command& command, std::chrono::nanoseconds input_time);
	/// @brief 切換駕駛的解籠選擇，回傳更新後的選擇狀態。
	bool toggle_missile_uncage();
	void stop();
	/// @brief 結束先前配置的武器使用與掛點切換輸入。
	void on_mode_changed();
	WeaponOperationEvent step(std::chrono::nanoseconds time);

private:
	void handle_missile_step(bool pressed, std::chrono::nanoseconds time);
	void submit_missile_step(MissileSelectionStep step);
	bool can_use_gun() const;
	bool can_release_weapon() const;
	void handle_gun(bool pressed);
	void handle_release(bool pressed);
	const ModeData& mode_;
	const WeaponUseData& execution_;
	WeaponOperationData& weapon_;
	StoresCommandQueue& stores_commands_;
	std::optional<std::chrono::nanoseconds> missile_step_pressed_at_;
	bool missile_type_step_sent_ = false;
	std::uint64_t reported_release_end_ = 0; ///< 最近已通知 FC 的正常投放結束要求。
};
}
