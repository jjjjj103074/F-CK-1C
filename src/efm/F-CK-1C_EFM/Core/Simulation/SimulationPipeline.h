#pragma once

#include "AircraftState.h"
#include "../Contracts/FrameContracts.h"

#include <array>
#include <memory>

namespace Core
{
namespace Systems
{
class AircraftDataSnapshot;
}

namespace Simulation
{
class AerodynamicsModel;
class GroundInteractionModel;
class MassPropertiesModel;
class PropulsionModel;

struct SimulationModels
{
	SimulationModels();
	~SimulationModels();
	SimulationModels(SimulationModels&&) noexcept;
	SimulationModels& operator=(SimulationModels&&) noexcept;

	SimulationModels(const SimulationModels&) = delete;
	SimulationModels& operator=(const SimulationModels&) = delete;

	std::unique_ptr<AerodynamicsModel> aerodynamics;
	std::unique_ptr<PropulsionModel> propulsion;
	std::unique_ptr<GroundInteractionModel> ground_interaction;
	std::unique_ptr<MassPropertiesModel> mass_properties;
};

struct SimulationFrameInput
{
	const Systems::AircraftDataSnapshot& aircraft;
	const AircraftState& observation;
	const FrameInput& frame;
	bool easy_flight = false;
};

struct SimulationResult
{
	ForceMomentOutput force_moment;
	std::array<double, kFrameEngineCount> thrust_force = {};
	MassDeltaResult mass_effect;
	double shake_amplitude = 0.0;
};

class SimulationPipeline final
{
public:
	explicit SimulationPipeline(SimulationModels models);
	~SimulationPipeline();

	SimulationPipeline(const SimulationPipeline&) = delete;
	SimulationPipeline& operator=(const SimulationPipeline&) = delete;

	const SimulationResult& step(const SimulationFrameInput& input);
	const SimulationResult& result() const;

private:
	struct Implementation;
	std::unique_ptr<Implementation> implementation_;
};
}
}
