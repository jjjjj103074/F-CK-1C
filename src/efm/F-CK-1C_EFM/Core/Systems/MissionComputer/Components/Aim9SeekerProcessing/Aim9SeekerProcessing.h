#pragma once

#include "../../Database/InputOutputData.h"
#include "../../Database/MissionData.h"

#include <chrono>

namespace Core::Systems::Mc
{
class Aim9SeekerProcessing final
{
public:
	Aim9SeekerProcessing(const MissionInputs& input,
	                     const ModeData& mode,
	                     const StoresData& stores,
	                     const SensorData& sensor,
	                     Aim9Data& aim9);
	void step(std::chrono::nanoseconds time, double dt_s);

private:
	void resolve_tracking();
	bool telemetry_active() const;
	const MissionInputs& input_;
	const ModeData& mode_;
	const StoresData& stores_;
	const SensorData& sensor_;
	Aim9Data& aim9_;
};
}
