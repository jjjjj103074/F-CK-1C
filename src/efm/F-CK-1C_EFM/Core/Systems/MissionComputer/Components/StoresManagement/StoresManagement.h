#pragma once

#include "../../Database/InputOutputData.h"
#include "../../Database/ExternalActionQueue.h"
#include "../../Database/MissionData.h"

#include <chrono>

namespace Core::Systems::Mc
{
class StoresManagement final
{
public:
	StoresManagement(const MissionInputs& input,
	                 const ModeData& mode,
	                 const WeaponOperationData& weapon,
	                 StoresData& stores,
	                 ExternalActionQueue& actions);
	void step(std::chrono::nanoseconds time, double dt_s);
	void record_missile_release();
	/// @brief 保留舊模式切換的武器循環要求；選站與確認仍由本元件管理。
	void request_weapon_cycle();

private:
	void update_gun_quantity(std::chrono::nanoseconds time);
	static bool is_air_to_air_missile(const WeaponStationSample& sample);
	void request_station(std::optional<int> station, bool force);
	const MissionInputs& input_;
	const ModeData& mode_;
	const WeaponOperationData& weapon_;
	StoresData& stores_;
	ExternalActionQueue& actions_;
	FireControlMode selection_mode_ = FireControlMode::Navigation;
	std::uint64_t rearm_revision_ = 0;
	std::chrono::nanoseconds gun_quantity_time_{};
	std::uint64_t gun_quantity_sequence_ = 0;
};
}
