#include "AerodynamicsModel.h"
#include "AerodynamicsPhysics.h"

#include <cmath>
#include <stdexcept>

namespace
{
constexpr std::size_t kPrimaryEffectCapacity = 7;
constexpr std::size_t kSupplementalEffectCapacity = 4;

void validate_primary_surface_positions(
	const Core::FlightControlActuatorState& primary)
{
	const double positions[] = {
		primary.symmetric_stabilator.position_rad,
		primary.differential_flaperon.position_rad,
		primary.rudder.position_rad
	};
	for (double position_rad : positions)
	{
		if (!std::isfinite(position_rad))
		{
			throw std::invalid_argument(
				"Aerodynamics input requires finite primary surface positions.");
		}
	}
}
}

namespace Core
{
namespace Simulation
{
struct AerodynamicsModel::Implementation
{
	explicit Implementation(const AerodynamicsConfig& model_config)
		: config(model_config)
	{
		validate_aerodynamics_config(config);
		result.primary_effects.reserve(kPrimaryEffectCapacity);
		result.supplemental_effects.reserve(kSupplementalEffectCapacity);
	}

	AerodynamicsPhysics::AerodynamicsFrameInput make_frame_input(
		const AerodynamicsModelInput& input) const;
	void update_conditions(const AerodynamicsModelInput& input);
	void record_primary(
		const AerodynamicsPhysics::AerodynamicsFrameInput& input);
	void record_supplemental(
		const AerodynamicsPhysics::AerodynamicsFrameInput& input);
	const AerodynamicsResult& step(const AerodynamicsModelInput& input);

	const AerodynamicsConfig config;
	AerodynamicsPhysics::AerodynamicsState state;
	AerodynamicsResult result;
};

AerodynamicsPhysics::AerodynamicsFrameInput
	AerodynamicsModel::Implementation::make_frame_input(
		const AerodynamicsModelInput& input) const
{
	const AircraftState& observation = input.observation;
	AerodynamicsPhysics::AerodynamicsFrameInput frame;
	frame.center_of_mass_body_m = observation.center_of_mass_body_m;
	frame.mach = observation.mach;
	frame.angle_of_attack_rad = observation.angle_of_attack_rad;
	frame.angle_of_attack_deg = observation.angle_of_attack_deg;
	frame.angle_of_slide_rad = observation.angle_of_slide_rad;
	frame.roll_rad = observation.roll_rad;
	frame.pitch_rate_rad_s = observation.pitch_rate_rad_s;
	frame.roll_rate_rad_s = observation.roll_rate_rad_s;
	frame.yaw_rate_rad_s = observation.yaw_rate_rad_s;
	frame.symmetric_stabilator_position_rad =
		input.primary.symmetric_stabilator.position_rad;
	frame.differential_flaperon_position_rad =
		input.primary.differential_flaperon.position_rad;
	frame.rudder_position_rad = input.primary.rudder.position_rad;
	frame.airbrake_position_normalized =
		input.secondary.airbrake_position_normalized;
	frame.flaps_position_normalized =
		input.secondary.flaps_position_normalized;
	frame.gear_position_normalized =
		input.landing_gear.position_normalized;
	frame.left_wing_integrity_0_1 = input.integrity.left_wing_0_1;
	frame.right_wing_integrity_0_1 = input.integrity.right_wing_0_1;
	frame.tail_integrity_0_1 = input.integrity.tail_0_1;
	frame.easy_flight = input.easy_flight;
	frame.on_ground = input.landing_gear.on_ground;
	frame.normal_acceleration_g = observation.normal_acceleration_g;
	return frame;
}

void AerodynamicsModel::Implementation::update_conditions(
	const AerodynamicsModelInput& input)
{
	const AircraftState& observation = input.observation;
	AerodynamicsPhysics::update_aerodynamic_conditions(
		state,
		config,
		{
			observation.center_of_mass_body_m,
			observation.atmosphere_density_kg_m3,
			observation.true_airspeed_mps,
			observation.mach,
			observation.angle_of_attack_deg,
			observation.angle_of_slide_deg,
			input.secondary.slats_position_normalized
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
		{ config, input },
		record_force);
}

void AerodynamicsModel::Implementation::record_supplemental(
	const AerodynamicsPhysics::AerodynamicsFrameInput& input)
{
	auto record_force = [this](
		const Common::Vec3& force,
		const Common::Vec3& position)
	{
		result.supplemental_effects.push_back(
			make_local_force_effect(force, position));
	};
	auto record_moment = [this](const Common::Vec3& moment)
	{
		result.supplemental_effects.push_back(
			make_local_moment_effect(moment));
	};
	AerodynamicsPhysics::apply_supplemental_aerodynamics(
		state,
		{ config, input },
		AerodynamicsPhysics::make_aerodynamic_sinks(
			record_force,
			record_moment));
}

const AerodynamicsResult& AerodynamicsModel::Implementation::step(
	const AerodynamicsModelInput& input)
{
	validate_primary_surface_positions(input.primary);
	result.primary_effects.clear();
	result.supplemental_effects.clear();
	update_conditions(input);
	const AerodynamicsPhysics::AerodynamicsFrameInput frame =
		make_frame_input(input);
	record_primary(frame);
	record_supplemental(frame);
	result.shake_amplitude_normalized =
		AerodynamicsPhysics::update_aerodynamic_shake(
			state, config, frame);
	return result;
}

AerodynamicsModel::AerodynamicsModel(const AerodynamicsConfig& config)
	: implementation_(std::make_unique<Implementation>(config))
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
