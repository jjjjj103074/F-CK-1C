#include "SimulationPipeline.h"

#include "../ForceMoment.h"
#include "../Systems/SystemPipeline.h"
#include "Models/Aerodynamics/AerodynamicsModel.h"
#include "Models/GroundInteraction/GroundInteractionModel.h"
#include "Models/MassProperties/MassPropertiesModel.h"
#include "Models/Propulsion/PropulsionModel.h"
#include "../../Data/AircraftConfig.h"

#include <vector>

namespace Core
{
namespace Simulation
{
namespace
{
class FrameAccumulator final
{
public:
	explicit FrameAccumulator(const Common::Vec3& center_of_mass)
	{
		output_.center_of_mass = center_of_mass;
	}

	void apply(const std::vector<ModelEffect>& effects)
	{
		for (const ModelEffect& effect : effects)
		{
			if (effect.type == ModelEffectType::LocalForce)
			{
				Core::add_local_force(
					output_.force,
					output_.moment,
					{ output_.center_of_mass, effect.value, effect.position });
				continue;
			}
			Core::add_local_moment(output_.moment, effect.value);
		}
	}

	const ForceMomentOutput& output() const
	{
		return output_;
	}

private:
	ForceMomentOutput output_;
};
}

struct SimulationPipeline::Implementation
{
	explicit Implementation(const Data::AircraftConfig& config)
		: aerodynamics(config.aerodynamics),
		propulsion({
			config.engine.mach_table,
			config.engine.max_thrust_table,
			config.engine.afterburner.thrust_factor,
			config.left_engine_position,
			config.right_engine_position
		}),
		ground_interaction(
			Data::make_ground_interaction_definition(config.suspension))
	{
	}

	AerodynamicsModelInput make_aerodynamics_input(
		const SimulationFrameInput& input) const;
	PropulsionModelInput make_propulsion_input(
		const SimulationFrameInput& input) const;
	GroundInteractionModelInput make_ground_input(
		const SimulationFrameInput& input,
		const PropulsionResult& propulsion_result) const;
	const SimulationResult& step(const SimulationFrameInput& input);

	AerodynamicsModel aerodynamics;
	PropulsionModel propulsion;
	GroundInteractionModel ground_interaction;
	MassPropertiesModel mass_properties;
	SimulationResult current_result;
};

AerodynamicsModelInput
	SimulationPipeline::Implementation::make_aerodynamics_input(
		const SimulationFrameInput& input) const
{
	return {
		input.aircraft.read(AircraftDataKeys::kPrimaryControlPosition),
		input.aircraft.read(AircraftDataKeys::kSecondaryControlPosition),
		input.aircraft.read(AircraftDataKeys::kLandingGearData),
		input.aircraft.read(AircraftDataKeys::kAirframeIntegrity),
		input.observation,
		input.easy_flight
	};
}

PropulsionModelInput
	SimulationPipeline::Implementation::make_propulsion_input(
		const SimulationFrameInput& input) const
{
	return {
		input.aircraft.read(AircraftDataKeys::kEngineData),
		input.observation,
		input.frame.max_power
	};
}

GroundInteractionModelInput
	SimulationPipeline::Implementation::make_ground_input(
		const SimulationFrameInput& input,
		const PropulsionResult& propulsion_result) const
{
	return {
		input.aircraft.read(AircraftDataKeys::kEngineData),
		input.aircraft.read(AircraftDataKeys::kLandingGearData),
		input.observation,
		input.frame.availability,
		propulsion_result
	};
}

const SimulationResult& SimulationPipeline::Implementation::step(
	const SimulationFrameInput& input)
{
	const AerodynamicsResult& aerodynamics_result =
		aerodynamics.step(make_aerodynamics_input(input));
	const PropulsionResult& propulsion_result =
		propulsion.step(make_propulsion_input(input));
	const GroundInteractionResult& ground_result =
		ground_interaction.step(make_ground_input(input, propulsion_result));
	const MassDeltaResult& mass_result = mass_properties.step(
		input.aircraft.read(AircraftDataKeys::kFuelData));

	FrameAccumulator accumulator(input.observation.center_of_mass);
	// Preserve the established accumulation order so the refactor does not
	// change floating-point results while model execution remains fixed above.
	accumulator.apply(aerodynamics_result.primary_effects);
	accumulator.apply(propulsion_result.effects);
	accumulator.apply(aerodynamics_result.limiter_effects);
	accumulator.apply(ground_result.effects);
	current_result.force_moment = accumulator.output();
	current_result.thrust_force = {
		propulsion_result.left_thrust_force,
		propulsion_result.right_thrust_force
	};
	current_result.mass_effect = mass_result;
	current_result.shake_amplitude = aerodynamics_result.shake_amplitude;
	return current_result;
}

SimulationPipeline::SimulationPipeline(const Data::AircraftConfig& config)
	: implementation_(std::make_unique<Implementation>(config))
{
}

SimulationPipeline::~SimulationPipeline() = default;

const SimulationResult& SimulationPipeline::step(
	const SimulationFrameInput& input)
{
	return implementation_->step(input);
}

const SimulationResult& SimulationPipeline::result() const
{
	return implementation_->current_result;
}
}
}
