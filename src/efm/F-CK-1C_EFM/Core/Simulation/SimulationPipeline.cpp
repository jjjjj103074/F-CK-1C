#include "SimulationPipeline.h"

#include "ForceMoment.h"
#include "../Contracts/Diagnostics.h"
#include "../Systems/SystemPipeline.h"
#include "Models/Aerodynamics/AerodynamicsModel.h"
#include "Models/GroundInteraction/GroundInteractionModel.h"
#include "Models/MassProperties/MassPropertiesModel.h"
#include "Models/Propulsion/PropulsionModel.h"

#include <stdexcept>
#include <utility>
#include <vector>

namespace Core
{
namespace Simulation
{
SimulationModels::SimulationModels() = default;
SimulationModels::~SimulationModels() = default;
SimulationModels::SimulationModels(SimulationModels&&) noexcept = default;
SimulationModels& SimulationModels::operator=(
	SimulationModels&&) noexcept = default;

namespace
{
constexpr const char* kModelStepOperation = "step";
constexpr const char* kAerodynamicsOwner = "aerodynamics";
constexpr const char* kPropulsionOwner = "propulsion";
constexpr const char* kGroundInteractionOwner = "ground_interaction";
constexpr const char* kMassPropertiesOwner = "mass_properties";
constexpr const char* kUnknownExceptionReason = "unknown C++ exception";

template <typename Action>
decltype(auto) invoke_model_action(
	const char* owner,
	const char* operation,
	Action&& action)
{
	try
	{
		return std::forward<Action>(action)();
	}
	catch (const ExecutionError&)
	{
		throw;
	}
	catch (const std::exception& error)
	{
		throw ExecutionError({
			ExecutionOwnerType::SimulationModel,
			owner,
			operation,
			error.what()
		});
	}
	catch (...)
	{
		throw ExecutionError({
			ExecutionOwnerType::SimulationModel,
			owner,
			operation,
			kUnknownExceptionReason
		});
	}
}

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
	explicit Implementation(SimulationModels models)
		: aerodynamics(std::move(models.aerodynamics)),
		propulsion(std::move(models.propulsion)),
		ground_interaction(std::move(models.ground_interaction)),
		mass_properties(std::move(models.mass_properties))
	{
		if (!aerodynamics || !propulsion ||
			!ground_interaction || !mass_properties)
		{
			throw std::invalid_argument(
				"SimulationPipeline requires every simulation model.");
		}
	}

	AerodynamicsModelInput make_aerodynamics_input(
		const SimulationFrameInput& input) const;
	PropulsionModelInput make_propulsion_input(
		const SimulationFrameInput& input) const;
	GroundInteractionModelInput make_ground_input(
		const SimulationFrameInput& input,
		const PropulsionResult& propulsion_result) const;
	const AerodynamicsResult& step_aerodynamics(
		const SimulationFrameInput& input);
	const PropulsionResult& step_propulsion(
		const SimulationFrameInput& input);
	const GroundInteractionResult& step_ground_interaction(
		const SimulationFrameInput& input,
		const PropulsionResult& propulsion_result);
	const MassDeltaResult& step_mass_properties(
		const SimulationFrameInput& input);
	const SimulationResult& step(const SimulationFrameInput& input);

	std::unique_ptr<AerodynamicsModel> aerodynamics;
	std::unique_ptr<PropulsionModel> propulsion;
	std::unique_ptr<GroundInteractionModel> ground_interaction;
	std::unique_ptr<MassPropertiesModel> mass_properties;
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
		propulsion_result.left_thrust_force +
			propulsion_result.right_thrust_force
	};
}

const AerodynamicsResult&
	SimulationPipeline::Implementation::step_aerodynamics(
		const SimulationFrameInput& input)
{
	return invoke_model_action(
		kAerodynamicsOwner,
		kModelStepOperation,
		[this, &input]() -> const AerodynamicsResult&
		{
			return aerodynamics->step(make_aerodynamics_input(input));
		});
}

const PropulsionResult&
	SimulationPipeline::Implementation::step_propulsion(
		const SimulationFrameInput& input)
{
	return invoke_model_action(
		kPropulsionOwner,
		kModelStepOperation,
		[this, &input]() -> const PropulsionResult&
		{
			return propulsion->step(make_propulsion_input(input));
		});
}

const GroundInteractionResult&
	SimulationPipeline::Implementation::step_ground_interaction(
		const SimulationFrameInput& input,
		const PropulsionResult& propulsion_result)
{
	return invoke_model_action(
		kGroundInteractionOwner,
		kModelStepOperation,
		[this, &input, &propulsion_result]() ->
			const GroundInteractionResult&
		{
			return ground_interaction->step(
				make_ground_input(input, propulsion_result));
		});
}

const MassDeltaResult&
	SimulationPipeline::Implementation::step_mass_properties(
		const SimulationFrameInput& input)
{
	return invoke_model_action(
		kMassPropertiesOwner,
		kModelStepOperation,
		[this, &input]() -> const MassDeltaResult&
		{
			return mass_properties->step(
				input.aircraft.read(AircraftDataKeys::kFuelData));
		});
}

const SimulationResult& SimulationPipeline::Implementation::step(
	const SimulationFrameInput& input)
{
	const AerodynamicsResult& aerodynamics_result =
		step_aerodynamics(input);
	const PropulsionResult& propulsion_result =
		step_propulsion(input);
	const GroundInteractionResult& ground_result =
		step_ground_interaction(input, propulsion_result);
	const MassDeltaResult& mass_result = step_mass_properties(input);

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

SimulationPipeline::SimulationPipeline(SimulationModels models)
	: implementation_(
		std::make_unique<Implementation>(std::move(models)))
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
