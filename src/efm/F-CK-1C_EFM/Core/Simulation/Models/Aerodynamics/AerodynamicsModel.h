#pragma once

#include "AerodynamicsConfig.h"
#include "../ModelEffect.h"
#include "../../AircraftState.h"
#include "../../../Contracts/AircraftData.h"

#include <memory>
#include <vector>

namespace Core
{
namespace Simulation
{
struct AerodynamicsModelInput
{
	const PrimaryControlPosition& primary;
	const SecondaryControlPosition& secondary;
	const LandingGearData& landing_gear;
	const AirframeIntegrity& integrity;
	const AircraftState& observation;
	bool easy_flight = false;
};

struct AerodynamicsResult
{
	std::vector<ModelEffect> primary_effects;
	std::vector<ModelEffect> limiter_effects;
	double shake_amplitude = 0.0;
};

class AerodynamicsModel final
{
public:
	explicit AerodynamicsModel(const AerodynamicsConfig& config);
	~AerodynamicsModel();

	AerodynamicsModel(const AerodynamicsModel&) = delete;
	AerodynamicsModel& operator=(const AerodynamicsModel&) = delete;

	const AerodynamicsResult& step(const AerodynamicsModelInput& input);

private:
	struct Implementation;
	std::unique_ptr<Implementation> implementation_;
};
}
}
