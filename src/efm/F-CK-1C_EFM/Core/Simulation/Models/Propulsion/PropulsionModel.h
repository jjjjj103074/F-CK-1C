#pragma once

#include "../ModelEffect.h"
#include "../../../AircraftState.h"
#include "../../../Contracts/AircraftData.h"
#include "../../../Contracts/FrameContracts.h"

#include <vector>

namespace Core
{
namespace Simulation
{
struct PropulsionModelDefinition
{
	const std::vector<double>& mach_table;
	const std::vector<double>& max_thrust_table;
	double afterburner_thrust_factor = 1.0;
	Common::Vec3 left_engine_position;
	Common::Vec3 right_engine_position;
};

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
	explicit PropulsionModel(const PropulsionModelDefinition& definition);

	const PropulsionResult& step(const PropulsionModelInput& input);

private:
	const std::vector<double>& mach_table_;
	const std::vector<double>& max_thrust_table_;
	double afterburner_thrust_factor_ = 1.0;
	Common::Vec3 left_engine_position_;
	Common::Vec3 right_engine_position_;
	PropulsionResult result_;
};
}
}
