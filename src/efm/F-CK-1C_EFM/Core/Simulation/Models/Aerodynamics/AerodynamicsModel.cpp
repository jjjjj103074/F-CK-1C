#include "AerodynamicsModel.h"
#include "AerodynamicsPhysics.h"

namespace
{
constexpr std::size_t kPrimaryEffectCapacity = 7;
constexpr std::size_t kLimiterEffectCapacity = 7;
}

namespace Core
{
namespace Simulation
{
struct AerodynamicsModel::Implementation
{
	explicit Implementation(
		const ::Data::AerodynamicsDefinition& model_definition)
		: definition(model_definition)
	{
		result.primary_effects.reserve(kPrimaryEffectCapacity);
		result.limiter_effects.reserve(kLimiterEffectCapacity);
	}

	AerodynamicsPhysics::AerodynamicsFrameInput make_frame_input(
		const AerodynamicsModelInput& input) const;
	void update_conditions(const AerodynamicsModelInput& input);
	void record_primary(
		const AerodynamicsPhysics::AerodynamicsFrameInput& input);
	void record_limiters(
		const AerodynamicsPhysics::AerodynamicsFrameInput& input);
	const AerodynamicsResult& step(const AerodynamicsModelInput& input);

	const ::Data::AerodynamicsDefinition& definition;
	AerodynamicsPhysics::AerodynamicsState state;
	AerodynamicsResult result;
};

AerodynamicsPhysics::AerodynamicsFrameInput
	AerodynamicsModel::Implementation::make_frame_input(
		const AerodynamicsModelInput& input) const
{
	const AircraftState& observation = input.observation;
	AerodynamicsPhysics::AerodynamicsFrameInput frame;
	frame.center_of_mass = observation.center_of_mass;
	frame.mach = observation.mach;
	frame.aoa = observation.aoa;
	frame.alpha_deg = observation.alpha;
	frame.aos = observation.aos;
	frame.roll = observation.roll;
	frame.pitch_rate = observation.pitch_rate;
	frame.roll_rate = observation.roll_rate;
	frame.yaw_rate = observation.yaw_rate;
	frame.elevator_command = input.primary.elevator;
	frame.aileron_command = input.primary.aileron;
	frame.rudder_command = input.primary.rudder;
	frame.airbrake_pos = input.secondary.airbrake;
	frame.flaps_pos = input.secondary.flaps;
	frame.gear_pos = input.landing_gear.position;
	frame.left_wing_integrity = input.integrity.left_wing;
	frame.right_wing_integrity = input.integrity.right_wing;
	frame.tail_integrity = input.integrity.tail;
	frame.easy_flight = input.easy_flight;
	frame.on_ground = input.landing_gear.on_ground;
	frame.g_force = observation.g;
	return frame;
}

void AerodynamicsModel::Implementation::update_conditions(
	const AerodynamicsModelInput& input)
{
	const AircraftState& observation = input.observation;
	AerodynamicsPhysics::update_aerodynamic_conditions(
		state,
		definition,
		{
			observation.center_of_mass,
			observation.atmosphere_density,
			observation.speed_scalar,
			observation.mach,
			observation.alpha,
			observation.beta,
			input.secondary.slats
		});
}

void AerodynamicsModel::Implementation::record_primary(
	const AerodynamicsPhysics::AerodynamicsFrameInput& input)
{
	auto record_force = [this](
		const Common::Vec3& force,
		const Common::Vec3& position)
	{
		result.primary_effects.push_back(
			make_local_force_effect(force, position));
	};
	AerodynamicsPhysics::apply_primary_aerodynamics(
		state,
		{ definition, input },
		record_force);
}

void AerodynamicsModel::Implementation::record_limiters(
	const AerodynamicsPhysics::AerodynamicsFrameInput& input)
{
	auto record_force = [this](
		const Common::Vec3& force,
		const Common::Vec3& position)
	{
		result.limiter_effects.push_back(
			make_local_force_effect(force, position));
	};
	auto record_moment = [this](const Common::Vec3& moment)
	{
		result.limiter_effects.push_back(
			make_local_moment_effect(moment));
	};
	AerodynamicsPhysics::apply_aerodynamic_limiters(
		state,
		{ definition, input },
		AerodynamicsPhysics::make_aerodynamic_sinks(
			record_force,
			record_moment));
}

const AerodynamicsResult& AerodynamicsModel::Implementation::step(
	const AerodynamicsModelInput& input)
{
	result.primary_effects.clear();
	result.limiter_effects.clear();
	update_conditions(input);
	const AerodynamicsPhysics::AerodynamicsFrameInput frame =
		make_frame_input(input);
	record_primary(frame);
	record_limiters(frame);
	result.shake_amplitude =
		AerodynamicsPhysics::update_aerodynamic_shake(
			state, definition, frame);
	return result;
}

AerodynamicsModel::AerodynamicsModel(
	const ::Data::AerodynamicsDefinition& definition)
	: implementation_(std::make_unique<Implementation>(definition))
{
}

AerodynamicsModel::~AerodynamicsModel() = default;

const AerodynamicsResult& AerodynamicsModel::step(
	const AerodynamicsModelInput& input)
{
	return implementation_->step(input);
}
}
}
