#pragma once

#include "../AircraftState.h"
#include "../Contracts/FrameContracts.h"

#include <array>
#include <memory>

namespace Data
{
struct AircraftConfig;
}

namespace Core
{
namespace Systems
{
class AircraftDataSnapshot;
}

namespace Simulation
{
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
	explicit SimulationPipeline(const Data::AircraftConfig& config);
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
