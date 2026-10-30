#pragma once

#include "../../../Database/InputOutputData.h"
#include "../../../Database/MissionData.h"

#include <chrono>

namespace Core::Systems::Mc::Internal
{
/// @brief Shared infrared missile support boundary; currently preserves the legacy AIM-9 prototype.
/// TODO: identify selected-weapon capabilities before supporting additional missile models.
class InfraredMissileSupport final
{
public:
	InfraredMissileSupport(const MissionInputs& input,
	                       const ModeData& mode,
	                       const StoresData& stores,
	                       const TargetingData& targeting,
	                       InfraredMissileData& missile);
	void step(std::chrono::nanoseconds time, double dt_s);

private:
	void resolve_tracking();
	bool telemetry_active() const;
	const MissionInputs& input_;
	const ModeData& mode_;
	const StoresData& stores_;
	const TargetingData& targeting_;
	InfraredMissileData& missile_;
};
}
