#pragma once

#include "../../../Database/MissionData.h"
#include "../../../../../Contracts/Commands.h"
#include "../../../Database/StoresCommandQueue.h"

namespace Core::Systems::Mc::Internal
{
/// @brief 判斷武器使用資格，提出開始／結束意圖並整理執行結果。
class WeaponOperation final
{
public:
	WeaponOperation(const ModeData& mode,
	                const RadarSnapshot& radar,
	                const SelectedWeaponState& selected,
	                const WeaponUseData& execution,
	                WeaponOperationData& weapon,
	                StoresCommandQueue& stores_commands);
	void handle(const Command& command, std::chrono::nanoseconds input_time);

	void stop();
	/// @brief 結束先前配置的武器使用與掛點切換輸入。
	void on_mode_changed();
	void step(std::chrono::nanoseconds time);

private:
	void handle_missile_step(bool pressed, std::chrono::nanoseconds time);
	void submit_missile_step(MissileSelectionStep step);
	bool can_use_gun() const;
	bool can_release_weapon() const;
	void handle_gun(bool pressed);
	void handle_release(bool pressed);
	const ModeData& mode_;
	const RadarSnapshot& radar_;
	const SelectedWeaponState& selected_;
	const WeaponUseData& execution_;
	WeaponOperationData& weapon_;
	StoresCommandQueue& stores_commands_;
	std::optional<std::chrono::nanoseconds> missile_step_pressed_at_;
	bool missile_type_step_sent_ = false;
};
}
