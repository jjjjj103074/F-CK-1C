#pragma once

#include "../../../../Data/AircraftDefinition.h"
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
struct AerodynamicsState
{
	Common::Vec3 left_wing_pos;
	Common::Vec3 right_wing_pos;
	Common::Vec3 tail_pos;
	Common::Vec3 elevator_pos;
	Common::Vec3 left_aileron_pos;
	Common::Vec3 right_aileron_pos;
	Common::Vec3 rudder_pos;
	bool force_positions_initialized = false;

	double dynamic_pressure = 0.0;
	double cy_alpha = 0.0;
	double cx_zero = 0.0;
	double cy_max = 0.0;
	double alpha_max_deg = 0.0;
	double roll_rate_max = 0.0;
	double wing_lift_coefficient = 0.0;
	double tail_lift_coefficient = 0.0;
	double lift_coefficient = 0.0;
	double induced_drag_coefficient = 0.0;
	double drag_coefficient = 0.0;

	Common::Vec3 left_wing_force;
	Common::Vec3 right_wing_force;
	Common::Vec3 tail_force;
	Common::Vec3 elevator_force;
	Common::Vec3 left_aileron_force;
	Common::Vec3 right_aileron_force;
	Common::Vec3 rudder_force;
	double roll_yaw_moment = 0.0;
	double roll_rate_limiter_moment = 0.0;
	double yaw_rate_limiter_moment = 0.0;
	double speed_limiter_force = 0.0;
	double airbrake_pitch_comp_moment = 0.0;
	double shake_amplitude = 0.0;
};

struct AerodynamicsFrameInput
{
	Common::Vec3 center_of_mass;
	double mach = 0.0;
	double aoa = 0.0;
	double alpha_deg = 0.0;
	double aos = 0.0;
	double roll = 0.0;
	double pitch_rate = 0.0;
	double roll_rate = 0.0;
	double yaw_rate = 0.0;
	double elevator_command = 0.0;
	double aileron_command = 0.0;
	double rudder_command = 0.0;
	double airbrake_pos = 0.0;
	double flaps_pos = 0.0;
	double gear_pos = 0.0;
	double left_wing_integrity = 1.0;
	double right_wing_integrity = 1.0;
	double tail_integrity = 1.0;
	bool easy_flight = false;
	bool on_ground = false;
	double g_force = 0.0;
};

struct AerodynamicConditionInput
{
	Common::Vec3 center_of_mass;
	double atmosphere_density = 0.0;
	double speed_scalar = 0.0;
	double mach = 0.0;
	double alpha_deg = 0.0;
	double beta_deg = 0.0;
	double slats_pos = 0.0;
};

struct AerodynamicsContext
{
	const ::Data::AerodynamicsDefinition& config;
	const AerodynamicsFrameInput& input;
};

inline void initialize_aerodynamic_force_positions(
	AerodynamicsState& state,
	const ::Data::AerodynamicsDefinition& config,
	const Common::Vec3& center_of_mass)
{
	state.left_wing_pos = Common::Vec3(
		center_of_mass.x - 0.7,
		center_of_mass.y + 0.5,
		-config.wingspan / 2.0);
	state.right_wing_pos = Common::Vec3(
		center_of_mass.x - 0.7,
		center_of_mass.y + 0.5,
		config.wingspan / 2.0);
	state.tail_pos = Common::Vec3(center_of_mass.x - 0.5, center_of_mass.y, 0.0);
	state.elevator_pos = Common::Vec3(-config.length / 2.0, center_of_mass.y, 0.0);
	state.left_aileron_pos = Common::Vec3(center_of_mass.x, center_of_mass.y, -config.wingspan * 0.5);
	state.right_aileron_pos = Common::Vec3(center_of_mass.x, center_of_mass.y, config.wingspan * 0.5);
	state.rudder_pos = Common::Vec3(-config.length / 2.0, config.height / 2.0, 0.0);
	state.force_positions_initialized = true;
}

inline void reset_aerodynamic_conditions(
	AerodynamicsState& state,
	const AerodynamicConditionInput& input)
{
	state.dynamic_pressure = 0.5 * input.atmosphere_density *
		input.speed_scalar * input.speed_scalar;
	state.cy_alpha = 0.0;
	state.cx_zero = 0.0;
	state.cy_max = 0.0;
	state.alpha_max_deg = 0.0;
	state.roll_rate_max = 0.0;
	state.wing_lift_coefficient = 0.0;
	state.tail_lift_coefficient = 0.0;
}

inline void interpolate_aerodynamic_conditions(
	AerodynamicsState& state,
	const ::Data::AerodynamicsDefinition& config,
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
		{ config.mach_table.data(), config.alpha_max_table.data(), table_size }, input.mach);
	state.roll_rate_max = Common::lerp(
		{ config.mach_table.data(), config.roll_rate_max_table.data(), table_size }, input.mach);
	state.cy_max += config.cy_flap * 0.4 * input.slats_pos;
}

inline void update_lift_coefficients(
	AerodynamicsState& state,
	const ::Data::AerodynamicsDefinition& config,
	const AerodynamicConditionInput& input)
{
	state.wing_lift_coefficient = state.cy_alpha * input.alpha_deg;
	if (state.wing_lift_coefficient > state.cy_max)
	{
		state.wing_lift_coefficient = state.cy_max;
	}
	if (state.wing_lift_coefficient < -state.cy_max)
	{
		state.wing_lift_coefficient = -state.cy_max;
	}
	state.tail_lift_coefficient =
		(0.5 * state.cy_alpha + config.cz_beta) * input.beta_deg;
	if (state.tail_lift_coefficient > state.cy_max)
	{
		state.tail_lift_coefficient = state.cy_max;
	}
	if (state.tail_lift_coefficient < -state.cy_max)
	{
		state.tail_lift_coefficient = -state.cy_max;
	}
	state.dynamic_pressure = 0.5 * input.atmosphere_density *
		input.speed_scalar * input.speed_scalar;
}

inline void update_aerodynamic_conditions(
	AerodynamicsState& state,
	const ::Data::AerodynamicsDefinition& config,
	const AerodynamicConditionInput& input)
{
	if (!state.force_positions_initialized)
	{
		initialize_aerodynamic_force_positions(state, config, input.center_of_mass);
	}
	if (!::Data::has_valid_aerodynamics_definition(config))
	{
		reset_aerodynamic_conditions(state, input);
		return;
	}
	interpolate_aerodynamic_conditions(state, config, input);
	update_lift_coefficients(state, config, input);
}

inline void update_wing_force_positions(
	AerodynamicsState& state,
	const ::Data::AerodynamicsDefinition& config,
	const AerodynamicsFrameInput& input)
{
	if ((std::fabs(input.alpha_deg) / state.alpha_max_deg) >= 0.75)
	{
		const double alpha_shift = Common::limit(
			std::pow(std::fabs(input.alpha_deg) / (state.alpha_max_deg * 1.1), 3.0) / 2000.0,
			0.0,
			config.length / 3.0);
		state.left_wing_pos.x = input.center_of_mass.x - 0.7 -
			(alpha_shift + Common::limit(-input.aos * 10.0, 0.0, 1.0));
		state.right_wing_pos.x = input.center_of_mass.x - 0.7 -
			(alpha_shift + Common::limit(input.aos * 10.0, 0.0, 1.0));
	}
	else
	{
		state.left_wing_pos.x = input.center_of_mass.x - 0.7;
		state.right_wing_pos.x = input.center_of_mass.x - 0.7;
	}
}

template <typename ForceSink>
inline void apply_wing_aerodynamics(
	AerodynamicsState& state,
	const AerodynamicsContext& context,
	ForceSink& add_force)
{
	const ::Data::AerodynamicsDefinition& config = context.config;
	const AerodynamicsFrameInput& input = context.input;
	const double q = state.dynamic_pressure;
	state.left_wing_force = Common::Vec3(
		-state.drag_coefficient * (std::sin(-input.aos / 2.0) + 1.0) * q * (config.wing_area / 2.0) * input.left_wing_integrity,
		state.lift_coefficient * (std::sin(-input.aos / 2.0) / 2.0 + 1.0) * q * (config.wing_area / 2.0) * input.left_wing_integrity,
		0.0);
	add_force(state.left_wing_force, state.left_wing_pos);
	state.right_wing_force = Common::Vec3(
		-state.drag_coefficient * (std::sin(input.aos / 2.0) + 1.0) * q * (config.wing_area / 2.0) * input.right_wing_integrity,
		state.lift_coefficient * (std::sin(input.aos / 2.0) / 2.0 + 1.0) * q * (config.wing_area / 2.0) * input.right_wing_integrity,
		0.0);
	add_force(state.right_wing_force, state.right_wing_pos);
}

template <typename ForceSink>
inline void apply_tail_aerodynamics(
	AerodynamicsState& state,
	const AerodynamicsContext& context,
	ForceSink& add_force)
{
	const ::Data::AerodynamicsDefinition& config = context.config;
	const AerodynamicsFrameInput& input = context.input;
	const double q = state.dynamic_pressure;
	state.tail_force = Common::Vec3(
		std::pow(-state.tail_lift_coefficient, 3.0) * std::sin(input.aoa) * (config.wing_area / 2.0) * q * input.tail_integrity,
		0.0,
		-state.tail_lift_coefficient * std::cos(input.aoa) * q * (config.wing_area / 2.0) * input.tail_integrity);
	add_force(state.tail_force, state.tail_pos);
}

template <typename ForceSink>
inline void apply_elevator_aerodynamics(
	AerodynamicsState& state,
	const AerodynamicsContext& context,
	ForceSink& add_force)
{
	const ::Data::AerodynamicsDefinition& config = context.config;
	const AerodynamicsFrameInput& input = context.input;
	const double q = state.dynamic_pressure;
	const double elevator_deflection =
		(-(Common::rescale(input.elevator_command + 0.15, Common::rad(-25.0), Common::rad(35.0))) * 18.0) *
		std::cos(input.aoa / 2.0);
	const double pitch_stability = (input.aoa + std::sin(input.aoa / 2.0) / 2.0) +
		(input.pitch_rate * 2.0);
	state.elevator_force = Common::Vec3(
		0.0,
		((elevator_deflection * Common::limit(
			1.0 - std::sqrt((input.mach + config.mach_max * 0.4) / 3.0),
			0.001,
			1.0)) +
			(pitch_stability * (input.mach / 2.0 + 1.0))) * q,
		0.0);
	add_force(state.elevator_force, state.elevator_pos);
}

template <typename ForceSink>
inline void apply_aileron_aerodynamics(
	AerodynamicsState& state,
	const AerodynamicsContext& context,
	ForceSink& add_force)
{
	const ::Data::AerodynamicsDefinition& config = context.config;
	const AerodynamicsFrameInput& input = context.input;
	const double q = state.dynamic_pressure;
	const double aileron_deflection = Common::rescale(
		input.aileron_command,
		Common::rad(-30.0),
		Common::rad(30.0)) * 4.0;
	const double roll_stability = -input.roll_rate *
		(((std::fabs(input.aoa + 0.5) * std::fabs(input.aos + 0.5)) + 1.0) * (5.0 / config.wingspan)) +
		(std::sin(input.roll) / 2.0 * std::fabs(input.aoa / 2.0));
	state.left_aileron_force = Common::Vec3(0.0, (aileron_deflection + roll_stability) * q, 0.0);
	state.right_aileron_force = Common::Vec3(0.0, -(aileron_deflection + roll_stability) * q, 0.0);
	add_force(state.left_aileron_force, state.left_aileron_pos);
	add_force(state.right_aileron_force, state.right_aileron_pos);
}

template <typename ForceSink>
inline void apply_rudder_aerodynamics(
	AerodynamicsState& state,
	const AerodynamicsContext& context,
	ForceSink& add_force)
{
	const AerodynamicsFrameInput& input = context.input;
	const double q = state.dynamic_pressure;
	const double rudder_deflection = Common::rescale(
		input.rudder_command,
		Common::rad(-30.0),
		Common::rad(30.0)) * 1.5;
	const double yaw_stability = -((input.aos * 2.0) + input.yaw_rate);
	state.rudder_force = Common::Vec3(0.0, 0.0, (rudder_deflection + yaw_stability) * q);
	add_force(state.rudder_force, state.rudder_pos);
}

template <typename ForceSink>
inline void apply_primary_aerodynamics(
	AerodynamicsState& state,
	const AerodynamicsContext& context,
	ForceSink add_force)
{
	const ::Data::AerodynamicsDefinition& config = context.config;
	const AerodynamicsFrameInput& input = context.input;
	state.lift_coefficient = state.wing_lift_coefficient + config.cy_zero +
		(config.cy_flap * input.flaps_pos);
	state.induced_drag_coefficient =
		(config.cx_lift_k * state.lift_coefficient * state.lift_coefficient) +
		(config.cx_alpha_k * input.aoa * input.aoa) +
		(config.cx_elevator_k * std::fabs(input.elevator_command));
	state.drag_coefficient = state.cx_zero +
		(config.cx_airbrake * input.airbrake_pos) +
		(config.cx_flap * input.flaps_pos) +
		(config.cx_gear * input.gear_pos) + state.induced_drag_coefficient;
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

template <typename MomentSink>
inline void apply_rate_limiters(
	AerodynamicsState& state,
	const AerodynamicsContext& context,
	MomentSink& add_moment)
{
	const AerodynamicsFrameInput& input = context.input;
	const double q = state.dynamic_pressure;
	state.roll_yaw_moment = -(input.roll_rate / 2.0) * (q + 1e5 * 0.5);
	add_moment(Common::Vec3(0.0, state.roll_yaw_moment, 0.0));
	state.roll_rate_limiter_moment = -input.roll_rate * Common::limit(
		std::pow(Common::limit(std::fabs(input.roll_rate) / (state.roll_rate_max + 0.1), 0.0001, 2.0), 6.0) *
		(q + q + 1e5 * 0.3),
		-1e7,
		1e7);
	add_moment(Common::Vec3(state.roll_rate_limiter_moment, 0.0, 0.0));
	state.yaw_rate_limiter_moment = -(input.yaw_rate + input.aos) * (q + 1e5 * 0.5);
	add_moment(Common::Vec3(0.0, state.yaw_rate_limiter_moment, 0.0));
}

template <typename ForceSink>
inline void apply_speed_limiter(
	AerodynamicsState& state,
	const AerodynamicsContext& context,
	ForceSink& add_force)
{
	const ::Data::AerodynamicsDefinition& config = context.config;
	const AerodynamicsFrameInput& input = context.input;
	const double q = state.dynamic_pressure;
	state.speed_limiter_force = 0.0;
	if (input.mach > config.mach_max)
	{
		const double over_mach = (input.mach - config.mach_max) / config.mach_max;
		state.speed_limiter_force = Common::limit(
			std::pow(over_mach * 3.0, 2.0) * (q * 0.35 + 25000.0),
			0.0,
			6e5);
	}
	add_force(Common::Vec3(-state.speed_limiter_force, 0.0, 0.0), input.center_of_mass);
}

template <typename MomentSink>
inline void apply_airbrake_compensation(
	AerodynamicsState& state,
	const AerodynamicsContext& context,
	MomentSink& add_moment)
{
	const ::Data::AerodynamicsDefinition& config = context.config;
	const AerodynamicsFrameInput& input = context.input;
	const double q = state.dynamic_pressure;
	const double mean_aerodynamic_chord = config.wing_area / config.wingspan;
	state.airbrake_pitch_comp_moment = config.airbrake_pitch_comp_k *
		input.airbrake_pos * q * config.wing_area * mean_aerodynamic_chord;
	add_moment(Common::Vec3(0.0, 0.0, state.airbrake_pitch_comp_moment));
}

template <typename ForceSink, typename MomentSink>
inline void apply_easy_flight_assist(
	AerodynamicsState& state,
	const AerodynamicsContext& context,
	AerodynamicSinks<ForceSink, MomentSink>& sinks)
{
	const AerodynamicsFrameInput& input = context.input;
	const double q = state.dynamic_pressure;
	if (input.easy_flight)
	{
		sinks.moment(Common::Vec3(
			-(input.roll_rate / 4.0) * (1.0 - std::sqrt(std::fabs(input.aileron_command))) * (1e5 + q * 0.5),
			-(input.yaw_rate + (std::sin(input.aos) / 2.0)) * (1.0 - std::sqrt(std::fabs(input.rudder_command))) * (1e5 + q * 0.5),
			-(input.pitch_rate + (std::sin(input.aoa) / 2.0)) * (1.0 - std::sqrt(std::fabs(input.elevator_command))) * (1e5 + q * 0.5)));
		sinks.force(
			Common::Vec3(0.0, 0.0, -input.rudder_command * (1e5 + q * 0.1)),
			Common::Vec3(input.center_of_mass.x - 0.2, input.center_of_mass.y, 0.0));
	}
}

template <typename ForceSink, typename MomentSink>
inline void apply_aerodynamic_limiters(
	AerodynamicsState& state,
	const AerodynamicsContext& context,
	AerodynamicSinks<ForceSink, MomentSink> sinks)
{
	apply_rate_limiters(state, context, sinks.moment);
	apply_speed_limiter(state, context, sinks.force);
	apply_airbrake_compensation(state, context, sinks.moment);
	apply_easy_flight_assist(state, context, sinks);
}

inline double update_aerodynamic_shake(
	AerodynamicsState& state,
	const ::Data::AerodynamicsDefinition& config,
	const AerodynamicsFrameInput& input)
{
	state.shake_amplitude = Common::limit(
		(config.cx_airbrake + 1.0) * input.airbrake_pos * input.mach,
		0.0,
		2.0) / 6.0;

	if (!input.on_ground)
	{
		if (std::fabs(input.alpha_deg) > 10.0)
		{
			state.shake_amplitude += (std::fabs(input.alpha_deg) - 10.0) / 100.0;
		}
		const double beta_deg = Common::deg(input.aos);
		if (std::fabs(beta_deg) > 10.0)
		{
			state.shake_amplitude += (std::fabs(beta_deg) - 10.0) / 100.0;
		}
		if (std::fabs(input.g_force) > 5.0)
		{
			state.shake_amplitude += (std::fabs(input.g_force) - 5.0) / 100.0;
		}
		if (input.mach > config.mach_max * 0.8)
		{
			state.shake_amplitude += (input.mach - config.mach_max * 0.8) / 2.0;
		}
	}

	return state.shake_amplitude;
}
}
}
}
