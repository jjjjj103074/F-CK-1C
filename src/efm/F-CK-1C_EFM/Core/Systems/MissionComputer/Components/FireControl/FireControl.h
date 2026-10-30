#pragma once

#include "Internal/Targeting.h"
#include "Internal/WeaponOperation.h"
#include "Internal/InfraredMissileSupport.h"

namespace Core::Systems::Mc
{
/// @brief MC 的火控入口，統一目標操作與武器使用程序；不包含雷達處理或顯示繪製。
class FireControl final
{
public:
	FireControl(const MissionInputs& input,
	            const ModeData& mode,
	            const StoresData& stores,
	            FireControlData& data,
	            ExternalActionQueue& actions,
	            StoresManagement& stores_management);
	FireControl(const FireControl&) = delete;
	FireControl& operator=(const FireControl&) = delete;
	FireControl(FireControl&&) = delete;
	FireControl& operator=(FireControl&&) = delete;
	void handle(const Command& command);
	void on_mode_selected(bool cycle_weapon);
	void on_master_arm_changed();
	void step(std::chrono::nanoseconds time, double dt_s);

	// 待後續設計：武器能力資料、特殊操作與瞄準／投放解算；本次不增加演算法或發射條件。

private:
	const ModeData& mode_;
	Internal::Targeting targeting_;
	Internal::WeaponOperation weapon_operation_;
	Internal::InfraredMissileSupport infrared_missile_support_;
};
}
