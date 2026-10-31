#pragma once

#include "../../ModeEvents.h"
#include "Internal/Targeting.h"
#include "Internal/WeaponOperation.h"
#include "Internal/InfraredMissileSupport.h"

namespace Core::Systems::Mc
{
/// @brief MC 火控入口，處理目標操作與武器使用程序。
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
	/// @brief 在 ModeData 更新後處理目標操作模式，保留重選時的武器循環意圖。
	void on_operating_mode_selected(const OperatingModeSelected& event);
	/// @brief 主保險選擇為 OFF/SIM 時停止武器程序。
	void on_master_arm_selected(const MasterArmSelected& event);
	void step(std::chrono::nanoseconds time, double dt_s);

private:
	Internal::Targeting targeting_;
	Internal::WeaponOperation weapon_operation_;
	Internal::InfraredMissileSupport infrared_missile_support_;
};
}
