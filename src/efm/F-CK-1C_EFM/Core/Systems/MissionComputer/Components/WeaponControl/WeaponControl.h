#pragma once

#include "../SensorCoordination/SensorCoordination.h"
#include "../StoresManagement/StoresManagement.h"

namespace Core::Systems::Mc
{
class WeaponControl final
{
public:
	WeaponControl(const MissionInputs& input,
	              const ModeData& mode,
	              const SensorData& sensor,
	              WeaponControlData& weapon,
	              ExternalActionQueue& actions,
	              SensorCoordination& sensors,
	              StoresManagement& stores);
	void handle(const Command& command);
	void stop();
	void step(std::chrono::nanoseconds time, double dt_s);

private:
	void finish_gun();
	void finish_pickle(bool consume);
	const MissionInputs& input_;
	const ModeData& mode_;
	const SensorData& sensor_;
	WeaponControlData& weapon_;
	ExternalActionQueue& actions_;
	SensorCoordination& sensors_;
	StoresManagement& stores_;
	TimedAction gun_;
	TimedAction pickle_;
};
}
