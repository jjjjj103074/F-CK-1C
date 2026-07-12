#pragma once

#include "../Common/Vec3.h"
#include <cstdio>

namespace Diagnostics
{
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
	char* buffer,
	size_t buffer_size,
	int element,
	double integrity,
	bool invincible)
{
	snprintf(
		buffer,
		buffer_size,
		"damage element=%d integrity=%.3f invincible=%d",
		element,
		integrity,
		invincible ? 1 : 0);
}

inline void format_engine_shutdown(
	char* buffer,
	size_t buffer_size,
	double internal_fuel,
	double altitude_asl,
	bool left_switch,
	bool right_switch)
{
	snprintf(
		buffer,
		buffer_size,
		"ENG_SHUTDOWN fuel=%.3f asl=%.3f left_sw=%d right_sw=%d",
		internal_fuel,
		altitude_asl,
		left_switch ? 1 : 0,
		right_switch ? 1 : 0);
}

inline void format_thrust_diagnostics(
	char* buffer,
	size_t buffer_size,
	const ThrustDiagnosticsSnapshot& snapshot)
{
	snprintf(
		buffer,
		buffer_size,
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
	char* buffer,
	size_t buffer_size,
	int index,
	double compression,
	double fx,
	double fy,
	double fz,
	double force_magnitude,
	bool wow)
{
	snprintf(
		buffer,
		buffer_size,
		"idx=%d comp=%.6f force=(%.3f, %.3f, %.3f) mag=%.3f wow=%d",
		index,
		compression,
		fx,
		fy,
		fz,
		force_magnitude,
		wow ? 1 : 0);
}

inline void format_suspension_animation(
	char* buffer,
	size_t buffer_size,
	int index,
	double compression,
	double draw_arg,
	double wheel_speed)
{
	snprintf(
		buffer,
		buffer_size,
		"susp idx=%d comp_norm=%.6f arg_val=%.3f speed=%.3f",
		index,
		compression,
		draw_arg,
		wheel_speed);
}
}
