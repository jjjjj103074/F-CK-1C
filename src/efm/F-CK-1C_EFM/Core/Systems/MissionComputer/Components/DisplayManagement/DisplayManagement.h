#pragma once

#include "../../Database/MissionData.h"
#include "../../../../Contracts/Commands.h"

#include <chrono>

namespace Core::Systems::Mc
{
class DisplayManagement final
{
public:
	DisplayManagement(const ModeData& mode,
	                  const SensorData& sensor,
	                  const StoresData& stores,
	                  const WeaponControlData& weapon,
	                  const Aim9Data& aim9,
	                  WeaponDisplayData& display);
	void observe(const Command& command);
	void step(std::chrono::nanoseconds time, double dt_s);

private:
	const ModeData& mode_;
	const SensorData& sensor_;
	const StoresData& stores_;
	const WeaponControlData& weapon_;
	const Aim9Data& aim9_;
	WeaponDisplayData& display_;
};
}
