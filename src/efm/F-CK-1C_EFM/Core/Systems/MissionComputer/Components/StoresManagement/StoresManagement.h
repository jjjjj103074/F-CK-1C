#pragma once

#include "../../Database/InputOutputData.h"
#include "../../Database/MissionData.h"

#include <chrono>

namespace Core::Systems::Mc
{
class StoresManagement final
{
public:
	StoresManagement(const MissionInputs& input, const ModeData& mode, StoresData& stores);
	void step(std::chrono::nanoseconds time, double dt_s);
	void record_missile_release();

private:
	static bool is_air_to_air_missile(const WeaponStationSample& sample);
	void request_station(std::optional<int> station, bool force);
	const MissionInputs& input_;
	const ModeData& mode_;
	StoresData& stores_;
	FireControlMode selection_mode_ = FireControlMode::Navigation;
	std::uint64_t rearm_revision_ = 0;
};
}
