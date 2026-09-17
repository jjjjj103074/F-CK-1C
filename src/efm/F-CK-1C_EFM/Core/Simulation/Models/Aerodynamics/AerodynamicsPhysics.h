#pragma once

#include "AerodynamicsConfig.h"
#include "../../../../Common/Clamp.h"
#include "../../../../Common/Interpolation.h"
#include "../../../../Common/Table.h"
#include "../../../../Common/Units.h"
#include "../../../../Common/Vec3.h"
#include <cmath>

namespace Core
{
namespace Simulation
{
namespace AerodynamicsPhysics
{
inline constexpr double kStabilatorTrimRad = Common::rad(5.25);
inline constexpr double kStabilatorEffectiveness = 21.6;
inline constexpr double kFlaperonEffectiveness = 4.0 * 30.0 / 22.0;

struct AerodynamicsState
{
	Common::Vec3 left_wing_position_body_m;
	Common::Vec3 right_wing_position_body_m;
	Common::Vec3 tail_position_body_m;
	Common::Vec3 elevator_position_body_m;
	Common::Vec3 left_aileron_position_body_m;
	Common::Vec3 right_aileron_position_body_m;
	Common::Vec3 rudder_position_body_m;
	bool force_positions_initialized = false;

	double dynamic_pressure_pa = 0.0;
	double cy_alpha = 0.0;
	double cx_zero = 0.0;
	double cy_max = 0.0;
	double alpha_max_deg = 0.0;
	double wing_lift_coefficient = 0.0;
	double tail_lift_coefficient = 0.0;
	double lift_coefficient = 0.0;
	double induced_drag_coefficient = 0.0;
	double drag_coefficient = 0.0;

	Common::Vec3 left_wing_force_body_n;
	Common::Vec3 right_wing_force_body_n;
	Common::Vec3 tail_force_body_n;
	Common::Vec3 elevator_force_body_n;
	Common::Vec3 left_aileron_force_body_n;
	Common::Vec3 right_aileron_force_body_n;
	Common::Vec3 rudder_force_body_n;
	double speed_limiter_force_n = 0.0;
	double airbrake_pitch_moment_nm = 0.0;
	double shake_amplitude_normalized = 0.0;
};

struct AerodynamicsFrameInput
{
	// Private physics-stage contract. Field suffixes define units; local-body
	// signs preserve the DCS/Core convention declared by FrameContracts.
	Common::Vec3 center_of_mass_body_m;
	double mach = 0.0;
	double angle_of_attack_rad = 0.0;
	double angle_of_attack_deg = 0.0;
	double angle_of_slide_rad = 0.0;
	double roll_rad = 0.0;
	double pitch_rate_rad_s = 0.0;
	double roll_rate_rad_s = 0.0;
	double yaw_rate_rad_s = 0.0;
	double symmetric_stabilator_position_rad = 0.0;
	double differential_flaperon_position_rad = 0.0;
	double rudder_position_rad = 0.0;
	double airbrake_position_normalized = 0.0;
	double flaps_position_normalized = 0.0;
	double gear_position_normalized = 0.0;
	double left_wing_integrity_0_1 = 1.0;
	double right_wing_integrity_0_1 = 1.0;
	double tail_integrity_0_1 = 1.0;
	bool easy_flight = false;
	bool on_ground = false;
	double normal_acceleration_g = 0.0;
};

struct AerodynamicConditionInput
{
	Common::Vec3 center_of_mass_body_m;
	double atmosphere_density_kg_m3 = 0.0;
	double true_airspeed_mps = 0.0;
	double mach = 0.0;
	double angle_of_attack_deg = 0.0;
	double angle_of_slide_deg = 0.0;
	double slats_position_normalized = 0.0;
};

struct AerodynamicsContext
{
	const Core::Simulation::AerodynamicsConfig& config;
	const AerodynamicsFrameInput& input;
};

inline void initialize_aerodynamic_force_positions(
	AerodynamicsState& state,
	const Core::Simulation::AerodynamicsConfig& config,
	const Common::Vec3& center_of_mass)
{
	state.left_wing_position_body_m = Common::Vec3(
		center_of_mass.x - 0.7,
		center_of_mass.y + 0.5,
		-config.wingspan_m / 2.0);
	state.right_wing_position_body_m = Common::Vec3(
		center_of_mass.x - 0.7,
		center_of_mass.y + 0.5,
		config.wingspan_m / 2.0);
	state.tail_position_body_m =
		Common::Vec3(center_of_mass.x - 0.5, center_of_mass.y, 0.0);
	state.elevator_position_body_m =
		Common::Vec3(-config.length_m / 2.0, center_of_mass.y, 0.0);
	state.left_aileron_position_body_m = Common::Vec3(
		center_of_mass.x, center_of_mass.y, -config.wingspan_m * 0.5);
	state.right_aileron_position_body_m = Common::Vec3(
		center_of_mass.x, center_of_mass.y, config.wingspan_m * 0.5);
	state.rudder_position_body_m = Common::Vec3(
		-config.length_m / 2.0, config.height_m / 2.0, 0.0);
	state.force_positions_initialized = true;
}

inline void interpolate_aerodynamic_conditions(
	AerodynamicsState& state,
	const Core::Simulation::AerodynamicsConfig& config,
	const AerodynamicConditionInput& input)
{
	const unsigned table_size = static_cast<unsigned>(config.mach_table.size());
	state.cy_alpha = Common::lerp(
		{ config.mach_table.data(), config.cy_alpha_table.data(), table_size }, input.mach);
	state.cx_zero = Common::lerp(
		{ config.mach_table.data(), config.cx_zero_table.data(), table_size }, input.mach);
	state.cy_max = Common::lerp(
		{ config.mach_table.data(), config.cy_max_table.data(), table_size }, input.mach);
	state.alpha_max_deg = Common::lerp(
		{ config.mach_table.data(), config.alpha_max_table_deg.data(), table_size }, input.mach);
	state.cy_max += config.cy_flap * 0.4 * input.slats_position_normalized;
}

inline void update_lift_coefficients(
	AerodynamicsState& state,
	const Core::Simulation::AerodynamicsConfig& config,
	const AerodynamicConditionInput& input)
{
	state.wing_lift_coefficient =
		state.cy_alpha * input.angle_of_attack_deg;
	if (state.wing_lift_coefficient > state.cy_max)
	{
		state.wing_lift_coefficient = state.cy_max;
	}
	if (state.wing_lift_coefficient < -state.cy_max)
	{
		state.wing_lift_coefficient = -state.cy_max;
	}
	state.tail_lift_coefficient =
		(0.5 * state.cy_alpha + config.cz_beta) *
		input.angle_of_slide_deg;
	if (state.tail_lift_coefficient > state.cy_max)
	{
		state.tail_lift_coefficient = state.cy_max;
	}
	if (state.tail_lift_coefficient < -state.cy_max)
	{
		state.tail_lift_coefficient = -state.cy_max;
	}
	state.dynamic_pressure_pa = 0.5 * input.atmosphere_density_kg_m3 *
		input.true_airspeed_mps * input.true_airspeed_mps;
}

inline void update_aerodynamic_conditions(
	AerodynamicsState& state,
	const Core::Simulation::AerodynamicsConfig& config,
	const AerodynamicConditionInput& input)
{
	if (!state.force_positions_initialized)
	{
		initialize_aerodynamic_force_positions(
			state, config, input.center_of_mass_body_m);
	}
	interpolate_aerodynamic_conditions(state, config, input);
	update_lift_coefficients(state, config, input);
}

inline void update_wing_force_positions(
	AerodynamicsState& state,
	const Core::Simulation::AerodynamicsConfig& config,
	const AerodynamicsFrameInput& input)
{
	// Preserve the legacy high-alpha force-migration curve: the final quarter
	// of usable alpha moves each wing load aft, while AoS adds a capped
	// left/right offset. These empirical ratios define one tuned curve rather
	// than independently configurable aircraft geometry.
	if ((std::fabs(input.angle_of_attack_deg) / state.alpha_max_deg) >= 0.75)
	{
		const double alpha_shift = Common::limit(
			std::pow(
				std::fabs(input.angle_of_attack_deg) /
					(state.alpha_max_deg * 1.1),
				3.0) / 2000.0,
			0.0,
			config.length_m / 3.0);
		state.left_wing_position_body_m.x =
			input.center_of_mass_body_m.x - 0.7 -
			(alpha_shift + Common::limit(
				-input.angle_of_slide_rad * 10.0, 0.0, 1.0));
		state.right_wing_position_body_m.x =
			input.center_of_mass_body_m.x - 0.7 -
			(alpha_shift + Common::limit(
				input.angle_of_slide_rad * 10.0, 0.0, 1.0));
	}
	else
	{
		state.left_wing_position_body_m.x =
			input.center_of_mass_body_m.x - 0.7;
		state.right_wing_position_body_m.x =
			input.center_of_mass_body_m.x - 0.7;
	}
}

template <typename ForceSink>
inline void apply_wing_aerodynamics(
	AerodynamicsState& state,
	const AerodynamicsContext& context,
	ForceSink& add_force)
{
	const Core::Simulation::AerodynamicsConfig& config = context.config;
	const AerodynamicsFrameInput& input = context.input;
	const double q = state.dynamic_pressure_pa;
	state.left_wing_force_body_n = Common::Vec3(
		-state.drag_coefficient *
			(std::sin(-input.angle_of_slide_rad / 2.0) + 1.0) * q *
			(config.wing_area_m2 / 2.0) * input.left_wing_integrity_0_1,
		state.lift_coefficient *
			(std::sin(-input.angle_of_slide_rad / 2.0) / 2.0 + 1.0) * q *
			(config.wing_area_m2 / 2.0) * input.left_wing_integrity_0_1,
		0.0);
	add_force(
		state.left_wing_force_body_n,
		state.left_wing_position_body_m);
	state.right_wing_force_body_n = Common::Vec3(
		-state.drag_coefficient *
			(std::sin(input.angle_of_slide_rad / 2.0) + 1.0) * q *
			(config.wing_area_m2 / 2.0) * input.right_wing_integrity_0_1,
		state.lift_coefficient *
			(std::sin(input.angle_of_slide_rad / 2.0) / 2.0 + 1.0) * q *
			(config.wing_area_m2 / 2.0) * input.right_wing_integrity_0_1,
		0.0);
	add_force(
		state.right_wing_force_body_n,
		state.right_wing_position_body_m);
}

template <typename ForceSink>
inline void apply_tail_aerodynamics(
	AerodynamicsState& state,
	const AerodynamicsContext& context,
	ForceSink& add_force)
{
	const Core::Simulation::AerodynamicsConfig& config = context.config;
	const AerodynamicsFrameInput& input = context.input;
	const double q = state.dynamic_pressure_pa;
	state.tail_force_body_n = Common::Vec3(
		std::pow(-state.tail_lift_coefficient, 3.0) *
			std::sin(input.angle_of_attack_rad) *
			(config.wing_area_m2 / 2.0) * q * input.tail_integrity_0_1,
		0.0,
		-state.tail_lift_coefficient * std::cos(input.angle_of_attack_rad) *
			q * (config.wing_area_m2 / 2.0) * input.tail_integrity_0_1);
	add_force(state.tail_force_body_n, state.tail_position_body_m);
}

template <typename ForceSink>
inline void apply_elevator_aerodynamics(
	AerodynamicsState& state,
	const AerodynamicsContext& context,
	ForceSink& add_force)
{
	const Core::Simulation::AerodynamicsConfig& config = context.config;
	const AerodynamicsFrameInput& input = context.input;
	const double q = state.dynamic_pressure_pa;
	const double elevator_deflection =
		(-(input.symmetric_stabilator_position_rad + kStabilatorTrimRad) *
			kStabilatorEffectiveness) *
		std::cos(input.angle_of_attack_rad / 2.0);
	const double pitch_stability =
		(input.angle_of_attack_rad +
			std::sin(input.angle_of_attack_rad / 2.0) / 2.0) +
		(input.pitch_rate_rad_s * 2.0);
	state.elevator_force_body_n = Common::Vec3(
		0.0,
		((elevator_deflection * Common::limit(
			1.0 - std::sqrt((input.mach + config.mach_max * 0.4) / 3.0),
			0.001,
			1.0)) +
			(pitch_stability * (input.mach / 2.0 + 1.0))) * q,
		0.0);
	add_force(state.elevator_force_body_n, state.elevator_position_body_m);
}

template <typename ForceSink>
inline void apply_aileron_aerodynamics(
	AerodynamicsState& state,
	const AerodynamicsContext& context,
	ForceSink& add_force)
{
	const Core::Simulation::AerodynamicsConfig& config = context.config;
	const AerodynamicsFrameInput& input = context.input;
	const double q = state.dynamic_pressure_pa;
	const double aileron_deflection =
		input.differential_flaperon_position_rad * kFlaperonEffectiveness;
	const double roll_stability = -input.roll_rate_rad_s *
		(((std::fabs(input.angle_of_attack_rad + 0.5) *
			std::fabs(input.angle_of_slide_rad + 0.5)) + 1.0) *
			(5.0 / config.wingspan_m)) +
		(std::sin(input.roll_rad) / 2.0 *
			std::fabs(input.angle_of_attack_rad / 2.0));
	state.left_aileron_force_body_n = Common::Vec3(
		0.0, (aileron_deflection + roll_stability) * q, 0.0);
	state.right_aileron_force_body_n = Common::Vec3(
		0.0, -(aileron_deflection + roll_stability) * q, 0.0);
	add_force(
		state.left_aileron_force_body_n,
		state.left_aileron_position_body_m);
	add_force(
		state.right_aileron_force_body_n,
		state.right_aileron_position_body_m);
}

template <typename ForceSink>
inline void apply_rudder_aerodynamics(
	AerodynamicsState& state,
	const AerodynamicsContext& context,
	ForceSink& add_force)
{
	const AerodynamicsFrameInput& input = context.input;
	const double q = state.dynamic_pressure_pa;
	const double rudder_deflection = input.rudder_position_rad * 1.5;
	const double yaw_stability =
		-((input.angle_of_slide_rad * 2.0) + input.yaw_rate_rad_s);
	state.rudder_force_body_n = Common::Vec3(
		0.0, 0.0, (rudder_deflection + yaw_stability) * q);
	add_force(state.rudder_force_body_n, state.rudder_position_body_m);
}

template <typename ForceSink>
inline void apply_primary_aerodynamics(
	AerodynamicsState& state,
	const AerodynamicsContext& context,
	ForceSink add_force)
{
	const Core::Simulation::AerodynamicsConfig& config = context.config;
	const AerodynamicsFrameInput& input = context.input;
	state.lift_coefficient = state.wing_lift_coefficient + config.cy_zero +
		(config.cy_flap * input.flaps_position_normalized);
	state.induced_drag_coefficient =
		(config.cx_lift_k * state.lift_coefficient * state.lift_coefficient) +
		(config.cx_alpha_k * input.angle_of_attack_rad *
			input.angle_of_attack_rad) +
		(config.cx_stabilator_per_rad *
			std::fabs(input.symmetric_stabilator_position_rad));
	state.drag_coefficient = state.cx_zero +
		(config.cx_airbrake * input.airbrake_position_normalized) +
		(config.cx_flap * input.flaps_position_normalized) +
		(config.cx_gear * input.gear_position_normalized) +
		state.induced_drag_coefficient;
	update_wing_force_positions(state, config, input);
	apply_wing_aerodynamics(state, context, add_force);
	apply_tail_aerodynamics(state, context, add_force);
	apply_elevator_aerodynamics(state, context, add_force);
	apply_aileron_aerodynamics(state, context, add_force);
	apply_rudder_aerodynamics(state, context, add_force);
}

template <typename ForceSink, typename MomentSink>
struct AerodynamicSinks
{
	ForceSink force;
	MomentSink moment;
};

template <typename ForceSink, typename MomentSink>
inline AerodynamicSinks<ForceSink, MomentSink> make_aerodynamic_sinks(
	ForceSink force,
	MomentSink moment)
{
	return { force, moment };
}

template <typename ForceSink>
inline void apply_easy_flight_speed_assist(
	AerodynamicsState& state,
	const AerodynamicsContext& context,
	ForceSink& add_force)
{
	const Core::Simulation::AerodynamicsConfig& config = context.config;
	const AerodynamicsFrameInput& input = context.input;
	const double q = state.dynamic_pressure_pa;
	state.speed_limiter_force_n = 0.0;
	if (!input.easy_flight)
	{
		return;
	}
	if (input.mach > config.mach_max)
	{
		const double over_mach = (input.mach - config.mach_max) / config.mach_max;
		state.speed_limiter_force_n = Common::limit(
			std::pow(over_mach * 3.0, 2.0) * (q * 0.35 + 25000.0),
			0.0,
			6e5);
	}
	add_force(
		Common::Vec3(-state.speed_limiter_force_n, 0.0, 0.0),
		input.center_of_mass_body_m);
}

template <typename MomentSink>
inline void apply_airbrake_aerodynamics(
	AerodynamicsState& state,
	const AerodynamicsContext& context,
	MomentSink& add_moment)
{
	const Core::Simulation::AerodynamicsConfig& config = context.config;
	const AerodynamicsFrameInput& input = context.input;
	const double q = state.dynamic_pressure_pa;
	const double mean_aerodynamic_chord_m =
		config.wing_area_m2 / config.wingspan_m;
	state.airbrake_pitch_moment_nm =
		config.airbrake_pitch_moment_coefficient *
		input.airbrake_position_normalized * q * config.wing_area_m2 *
		mean_aerodynamic_chord_m;
	add_moment(Common::Vec3(
		0.0, 0.0, state.airbrake_pitch_moment_nm));
}

template <typename ForceSink, typename MomentSink>
inline void apply_easy_flight_assist(
	AerodynamicsState& state,
	const AerodynamicsContext& context,
	AerodynamicSinks<ForceSink, MomentSink>& sinks)
{
	const AerodynamicsFrameInput& input = context.input;
	const Core::Simulation::AerodynamicsConfig& config = context.config;
	const double q = state.dynamic_pressure_pa;
	if (input.easy_flight)
	{
		const double roll_ratio = Common::limit(
			std::fabs(input.differential_flaperon_position_rad) /
				config.easy_flight_flaperon_limit_rad, 0.0, 1.0);
		const double yaw_ratio = Common::limit(
			std::fabs(input.rudder_position_rad) /
				config.easy_flight_rudder_limit_rad, 0.0, 1.0);
		const double pitch_ratio = Common::limit(
			std::fabs(input.symmetric_stabilator_position_rad) /
				config.easy_flight_stabilator_limit_rad, 0.0, 1.0);
		sinks.moment(Common::Vec3(
			-(input.roll_rate_rad_s / 4.0) *
				(1.0 - std::sqrt(roll_ratio)) *
					(1e5 + q * 0.5),
			-(input.yaw_rate_rad_s +
				(std::sin(input.angle_of_slide_rad) / 2.0)) *
				(1.0 - std::sqrt(yaw_ratio)) * (1e5 + q * 0.5),
			-(input.pitch_rate_rad_s +
				(std::sin(input.angle_of_attack_rad) / 2.0)) *
				(1.0 - std::sqrt(pitch_ratio)) *
				(1e5 + q * 0.5)));
		sinks.force(
			Common::Vec3(
				0.0,
				0.0,
				-(input.rudder_position_rad /
					config.easy_flight_rudder_limit_rad) *
					(1e5 + q * 0.1)),
			Common::Vec3(
				input.center_of_mass_body_m.x - 0.2,
				input.center_of_mass_body_m.y,
				0.0));
	}
}

template <typename ForceSink, typename MomentSink>
inline void apply_supplemental_aerodynamics(
	AerodynamicsState& state,
	const AerodynamicsContext& context,
	AerodynamicSinks<ForceSink, MomentSink> sinks)
{
	apply_easy_flight_speed_assist(state, context, sinks.force);
	apply_airbrake_aerodynamics(state, context, sinks.moment);
	apply_easy_flight_assist(state, context, sinks);
}

inline double update_aerodynamic_shake(
	AerodynamicsState& state,
	const Core::Simulation::AerodynamicsConfig& config,
	const AerodynamicsFrameInput& input)
{
	state.shake_amplitude_normalized = Common::limit(
		(config.cx_airbrake + 1.0) * input.airbrake_position_normalized *
		input.mach,
		0.0,
		2.0) / 6.0;

	if (!input.on_ground)
	{
		if (std::fabs(input.angle_of_attack_deg) > 10.0)
		{
			state.shake_amplitude_normalized +=
				(std::fabs(input.angle_of_attack_deg) - 10.0) / 100.0;
		}
		const double beta_deg = Common::deg(input.angle_of_slide_rad);
		if (std::fabs(beta_deg) > 10.0)
		{
			state.shake_amplitude_normalized +=
				(std::fabs(beta_deg) - 10.0) / 100.0;
		}
		if (std::fabs(input.normal_acceleration_g) > 5.0)
		{
			state.shake_amplitude_normalized +=
				(std::fabs(input.normal_acceleration_g) - 5.0) / 100.0;
		}
		if (input.mach > config.mach_max * 0.8)
		{
			state.shake_amplitude_normalized +=
				(input.mach - config.mach_max * 0.8) / 2.0;
		}
	}

	return state.shake_amplitude_normalized;
}
}
}
}
