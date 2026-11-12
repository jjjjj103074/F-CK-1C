#pragma once
#include "../../ModeEvents.h"
#include "Internal/Targeting.h"
#include "Internal/WeaponOperation.h"
namespace Core::Systems::Mc
{
/// @brief 火控組裝與操作分派；目標流程及武器流程各自管理狀態。
class FireControl final
{
public:
	FireControl(const ModeData& mode,
	            const RadarSnapshot& radar,
	            const WeaponUseData& execution,
	            FireControlData& data,
	            RadarRequestQueue& radar_requests,
	            StoresCommandQueue& stores_commands);
	FireControl(const FireControl&) = delete;
	FireControl& operator=(const FireControl&) = delete;
	FireControl(FireControl&&) = delete;
	FireControl& operator=(FireControl&&) = delete;
	void handle(const Command& command);
	void on_operating_mode_selected(const OperatingModeSelected& event);
	void on_master_arm_selected(const MasterArmSelected& event);
	void step(std::chrono::nanoseconds time, double dt_s);

private:
	Internal::Targeting targeting_;
	Internal::WeaponOperation weapon_operation_;
};
}
