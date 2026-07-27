#pragma once

#include "PropulsionConfig.h"
#include "../ModelEffect.h"
#include "../../AircraftState.h"
#include "../../../Contracts/AircraftData.h"
#include "../../../Contracts/FrameContracts.h"

#include <vector>

namespace Core
{
namespace Simulation
{
struct PropulsionModelInput
{
	const EngineData& engines;
	const AircraftState& observation;
	const MaxPowerCommand& max_power;
};

struct PropulsionResult
{
	std::vector<ModelEffect> effects;
	double left_thrust_force = 0.0;
	double right_thrust_force = 0.0;
};

class PropulsionModel final
{
public:
	explicit PropulsionModel(const PropulsionConfig& config);

	const PropulsionResult& step(const PropulsionModelInput& input);

private:
	const PropulsionConfig config_;
	PropulsionResult result_;
};
}
}
