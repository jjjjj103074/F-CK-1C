#pragma once

#include "../Common/Vec3.h"
#include <cstdio>

namespace Diagnostics
{
struct DiagnosticOutput
{
	char* data = nullptr;
	size_t capacity = 0;
};

struct DamageEventSnapshot
{
	int element = 0;
	double integrity = 0.0;
	bool invincible = false;
};

struct EngineShutdownSnapshot
{
	double internal_fuel = 0.0;
	double altitude_asl = 0.0;
	bool left_switch = false;
	bool right_switch = false;
};

struct SuspensionFeedbackSnapshot
{
	int index = 0;
	double compression = 0.0;
	Common::Vec3 force;
	double force_magnitude = 0.0;
	bool wow = false;
};

struct SuspensionAnimationSnapshot
{
	int index = 0;
	double compression = 0.0;
	double draw_arg = 0.0;
	double wheel_speed = 0.0;
};

struct ThrustDiagnosticsSnapshot
{
	double left_thrust = 0.0;
	double right_thrust = 0.0;
	Common::Vec3 net_moment;
	double suspension_force[3] = { 0.0, 0.0, 0.0 };
	double maxpower_ready = 0.0;
	double maxpower_value = 0.0;
	bool left_engine_switch = false;
	bool right_engine_switch = false;
	double left_throttle_input = 0.0;
	double right_throttle_input = 0.0;
	double left_throttle_output = 0.0;
	double right_throttle_output = 0.0;
	double left_power_readout = 0.0;
	double right_power_readout = 0.0;
	double left_wing_integrity = 0.0;
	double right_wing_integrity = 0.0;
	double left_engine_integrity = 0.0;
	double right_engine_integrity = 0.0;
	double internal_fuel = 0.0;
};

inline void format_damage_event(
	const DiagnosticOutput& output,
	const DamageEventSnapshot& snapshot)
{
	snprintf(
		output.data,
		output.capacity,
		"damage element=%d integrity=%.3f invincible=%d",
		snapshot.element,
		snapshot.integrity,
		snapshot.invincible ? 1 : 0);
}

inline void format_engine_shutdown(
	const DiagnosticOutput& output,
	const EngineShutdownSnapshot& snapshot)
{
	snprintf(
		output.data,
		output.capacity,
		"ENG_SHUTDOWN fuel=%.3f asl=%.3f left_sw=%d right_sw=%d",
		snapshot.internal_fuel,
		snapshot.altitude_asl,
		snapshot.left_switch ? 1 : 0,
		snapshot.right_switch ? 1 : 0);
}

inline void format_thrust_diagnostics(
	const DiagnosticOutput& output,
	const ThrustDiagnosticsSnapshot& snapshot)
{
	snprintf(
		output.data,
		output.capacity,
		"THRUST L=%.3f R=%.3f NETM=(%.3f,%.3f,%.3f) SUSP=(%.3f,%.3f,%.3f) MAXPWR ready=%.1f sw=%.1f ENGSW=(%d,%d) THRIN=(%.3f,%.3f) THROUT=(%.3f,%.3f) CORE=(%.3f,%.3f) INTEG wing=(%.3f,%.3f) eng=(%.3f,%.3f) FUEL=%.1f",
		snapshot.left_thrust,
		snapshot.right_thrust,
		snapshot.net_moment.x,
		snapshot.net_moment.y,
		snapshot.net_moment.z,
		snapshot.suspension_force[0],
		snapshot.suspension_force[1],
		snapshot.suspension_force[2],
		snapshot.maxpower_ready,
		snapshot.maxpower_value,
		snapshot.left_engine_switch ? 1 : 0,
		snapshot.right_engine_switch ? 1 : 0,
		snapshot.left_throttle_input,
		snapshot.right_throttle_input,
		snapshot.left_throttle_output,
		snapshot.right_throttle_output,
		snapshot.left_power_readout,
		snapshot.right_power_readout,
		snapshot.left_wing_integrity,
		snapshot.right_wing_integrity,
		snapshot.left_engine_integrity,
		snapshot.right_engine_integrity,
		snapshot.internal_fuel);
}

inline void format_suspension_feedback(
	const DiagnosticOutput& output,
	const SuspensionFeedbackSnapshot& snapshot)
{
	snprintf(
		output.data,
		output.capacity,
		"idx=%d comp=%.6f force=(%.3f, %.3f, %.3f) mag=%.3f wow=%d",
		snapshot.index,
		snapshot.compression,
		snapshot.force.x,
		snapshot.force.y,
		snapshot.force.z,
		snapshot.force_magnitude,
		snapshot.wow ? 1 : 0);
}

inline void format_suspension_animation(
	const DiagnosticOutput& output,
	const SuspensionAnimationSnapshot& snapshot)
{
	snprintf(
		output.data,
		output.capacity,
		"susp idx=%d comp_norm=%.6f arg_val=%.3f speed=%.3f",
		snapshot.index,
		snapshot.compression,
		snapshot.draw_arg,
		snapshot.wheel_speed);
}
}
