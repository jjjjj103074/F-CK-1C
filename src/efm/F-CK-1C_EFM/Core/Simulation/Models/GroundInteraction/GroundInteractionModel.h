#pragma once

#include "GroundInteractionConfig.h"
#include "../ModelEffect.h"
#include "../../AircraftState.h"
#include "../../../Contracts/AircraftData.h"
#include "../../../Contracts/FrameContracts.h"

#include <memory>
#include <vector>

namespace Core
{
namespace Simulation
{
struct GroundInteractionModelInput
{
	const EngineData& engines;
	const LandingGearData& landing_gear;
	const AircraftState& observation;
	const FrameDataAvailability& availability;
	double total_thrust_force = 0.0;
};

struct GroundInteractionResult
{
	std::vector<ModelEffect> effects;
	bool used_fallback = false;
};

class GroundInteractionModel final
{
public:
	explicit GroundInteractionModel(const GroundInteractionConfig& config);
	~GroundInteractionModel();

	GroundInteractionModel(const GroundInteractionModel&) = delete;
	GroundInteractionModel& operator=(const GroundInteractionModel&) = delete;

	const GroundInteractionResult& step(
		const GroundInteractionModelInput& input);

private:
	struct Implementation;
	std::unique_ptr<Implementation> implementation_;
};
}
}
