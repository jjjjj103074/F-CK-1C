#pragma once

#include "DebugLogger.h"
#include "TextBuffer.h"
#include "../Common/Clamp.h"
#include "../Common/Vec3.h"
#include <cmath>
#include <cstdio>
#include <cstring>

namespace Diagnostics
{
static const int kDiagnosticWheelCount = 3;

struct SuspensionDiagnosticsConfig
{
	bool suspension_probe_enabled = true;
	double suspension_probe_interval = 0.25;
	bool startup_probe_enabled = true;
	double startup_probe_duration = 5.0;
	double startup_force_epsilon = 1e-3;
	int ground_log_decimation = 20;

	bool use_modelviewer_nodes = false;
	bool geometry_test = false;
	double radius_add = 0.0;
	double wheel_y_offset = 0.0;
	char test_mark[128] = "SUSP_TEST_MARK_NOT_FOUND";
	const char* wheel_nodes[kDiagnosticWheelCount] = { "WHEEL_F", "WHEEL_L", "WHEEL_R" };
	double final_wheel_radius[kDiagnosticWheelCount] = { 0.0, 0.0, 0.0 };
	Common::Vec3 final_wheel_pos[kDiagnosticWheelCount];
	const char* active_collision_shell = "";
	const char* suspension_mode = "";
	bool fallback_enabled = false;
	const char* build_date = "";
	const char* build_time = "";
};

struct SuspensionDiagnosticsState
{
	int fallback_log_decimation = 0;
	bool ground_config_logged = false;
	double suspension_probe_timer = 0.0;
	unsigned long long startup_frame_index = 0;
	double startup_probe_elapsed = 0.0;
	bool startup_seen_force = false;
	bool startup_logged_first_force = false;
	bool startup_logged_zero_after_force = false;
	bool startup_seen_any_wow = false;
	bool startup_logged_all_wow_zero = false;
	bool startup_seen_any_compression = false;
	bool startup_logged_compression_all_zero = false;
};

struct SuspensionDiagnosticsSnapshot
{
	double simulation_time = 0.0;
	double altitude_agl = 0.0;
	double surface_height = 0.0;
	double surface_height_with_objects = 0.0;
	unsigned surface_type = 0;
	double vertical_velocity = 0.0;
	double pitch_deg = 0.0;
	double roll_deg = 0.0;
	double current_mass = 0.0;
	double gear_pos = 0.0;
	bool feedback_valid[kDiagnosticWheelCount] = { false, false, false };
	bool wow[kDiagnosticWheelCount] = { false, false, false };
	double compression[kDiagnosticWheelCount] = { 0.0, 0.0, 0.0 };
	Common::Vec3 force_vec[kDiagnosticWheelCount];
	double force_magnitude[kDiagnosticWheelCount] = { 0.0, 0.0, 0.0 };
	bool fallback_wow[kDiagnosticWheelCount] = { false, false, false };
	double fallback_compression[kDiagnosticWheelCount] = { 0.0, 0.0, 0.0 };
	double fallback_ground_force = 0.0;
	double left_throttle_output = 0.0;
	double right_throttle_output = 0.0;
	double left_thrust = 0.0;
	double right_thrust = 0.0;
	Common::Vec3 velocity_world;
	Common::Vec3 velocity_body;
	Common::Vec3 angular_velocity_world;
	Common::Vec3 angular_velocity_body;
	double brake = 0.0;
	double brake_left = 0.0;
	double brake_right = 0.0;
	double yaw_input = 0.0;
	double rudder_command = 0.0;
	double nose_wheel_command = 0.0;
	double nose_wheel_draw_arg = 0.0;
	bool nose_turn_enabled = false;
};

inline bool diagnostic_any_wow(const SuspensionDiagnosticsSnapshot& snapshot)
{
	return snapshot.wow[0] || snapshot.wow[1] || snapshot.wow[2];
}

inline bool diagnostic_any_feedback(const SuspensionDiagnosticsSnapshot& snapshot)
{
	return snapshot.feedback_valid[0] || snapshot.feedback_valid[1] || snapshot.feedback_valid[2];
}

inline bool diagnostic_any_fallback_wow(const SuspensionDiagnosticsSnapshot& snapshot)
{
	return snapshot.fallback_wow[0] || snapshot.fallback_wow[1] || snapshot.fallback_wow[2];
}

inline double diagnostic_total_force(const SuspensionDiagnosticsSnapshot& snapshot)
{
	return snapshot.force_magnitude[0] + snapshot.force_magnitude[1] + snapshot.force_magnitude[2];
}

inline void reset_startup_suspension_probe(SuspensionDiagnosticsState& state)
{
	state.startup_frame_index = 0;
	state.startup_probe_elapsed = 0.0;
	state.startup_seen_force = false;
	state.startup_logged_first_force = false;
	state.startup_logged_zero_after_force = false;
	state.startup_seen_any_wow = false;
	state.startup_logged_all_wow_zero = false;
	state.startup_seen_any_compression = false;
	state.startup_logged_compression_all_zero = false;
}

struct SuspensionDiagnosticFrame
{
	const SuspensionDiagnosticsConfig& config;
	const SuspensionDiagnosticsSnapshot& snapshot;
	double dt = 0.0;
};

struct StartupProbeStatus
{
	double total_force = 0.0;
	bool any_force = false;
	bool any_wow = false;
	bool any_compression = false;
	char tags[256] = "";
};

template <typename DebugSink, typename ProbeSink>
struct SuspensionDiagnosticSinks
{
	DebugSink debug;
	ProbeSink probe;
};

template <typename DebugSink, typename ProbeSink>
inline SuspensionDiagnosticSinks<DebugSink, ProbeSink> make_diagnostic_sinks(
	DebugSink debug,
	ProbeSink probe)
{
	return { debug, probe };
}

inline void append_ground_configuration(
	TextBufferWriter& writer,
	const SuspensionDiagnosticsConfig& config)
{
	writer.append(
		"GROUND_CFG SUSP_TEST_MARK=%s efm_session=%s_%s "
		"SUSP_USE_MODELVIEWER_WHEEL_NODES=%d SUSP_GEOMETRY_TEST=%d "
		"radius_add=%.2f wheel_y_offset=%.2f gear_nodes=%s/%s/%s "
		"final_wheel_radius=%.4f/%.4f/%.4f "
		"final_wheel_pos=(%.3f,%.3f,%.3f)/(%.3f,%.3f,%.3f)/(%.3f,%.3f,%.3f) "
		"active_collision_shell=%s suspension_mode=%s fallback=%d",
		config.test_mark, config.build_date, config.build_time,
		config.use_modelviewer_nodes ? 1 : 0, config.geometry_test ? 1 : 0,
		config.radius_add, config.wheel_y_offset, config.wheel_nodes[0],
		config.wheel_nodes[1], config.wheel_nodes[2], config.final_wheel_radius[0],
		config.final_wheel_radius[1], config.final_wheel_radius[2],
		config.final_wheel_pos[0].x, config.final_wheel_pos[0].y,
		config.final_wheel_pos[0].z, config.final_wheel_pos[1].x,
		config.final_wheel_pos[1].y, config.final_wheel_pos[1].z,
		config.final_wheel_pos[2].x, config.final_wheel_pos[2].y,
		config.final_wheel_pos[2].z, config.active_collision_shell,
		config.suspension_mode, config.fallback_enabled ? 1 : 0);
}

template <typename DebugSink, typename ProbeSink>
inline void log_ground_configuration_once(
	SuspensionDiagnosticsState& state,
	const SuspensionDiagnosticsConfig& config,
	SuspensionDiagnosticSinks<DebugSink, ProbeSink> sinks)
{
	if (state.ground_config_logged)
	{
		return;
	}
	char buffer[1536];
	TextBufferWriter writer(buffer, sizeof(buffer));
	append_ground_configuration(writer, config);
	sinks.debug(buffer);
	sinks.probe(buffer);
	state.ground_config_logged = true;
}

inline bool startup_has_compression(const SuspensionDiagnosticsSnapshot& snapshot)
{
	return std::fabs(snapshot.compression[0]) > 1e-5 ||
		std::fabs(snapshot.compression[1]) > 1e-5 ||
		std::fabs(snapshot.compression[2]) > 1e-5;
}

inline void update_startup_force_status(
	SuspensionDiagnosticsState& state,
	StartupProbeStatus& status)
{
	if (status.any_force)
	{
		state.startup_seen_force = true;
		if (!state.startup_logged_first_force)
		{
			append_tag(status.tags, sizeof(status.tags), "FIRST_FORCE_FRAME");
			state.startup_logged_first_force = true;
		}
	}
	else if (state.startup_seen_force && !state.startup_logged_zero_after_force)
	{
		append_tag(status.tags, sizeof(status.tags), "FIRST_ZERO_FORCE_AFTER_FORCE");
		state.startup_logged_zero_after_force = true;
	}
}

inline void update_startup_wow_status(
	SuspensionDiagnosticsState& state,
	StartupProbeStatus& status)
{
	if (status.any_wow)
	{
		state.startup_seen_any_wow = true;
	}
	else if (state.startup_seen_any_wow && !state.startup_logged_all_wow_zero)
	{
		append_tag(status.tags, sizeof(status.tags), "FIRST_ALL_WOW_ZERO");
		state.startup_logged_all_wow_zero = true;
	}
}

inline void update_startup_compression_status(
	SuspensionDiagnosticsState& state,
	StartupProbeStatus& status)
{
	if (status.any_compression)
	{
		state.startup_seen_any_compression = true;
	}
	else if (state.startup_seen_any_compression &&
		!state.startup_logged_compression_all_zero)
	{
		append_tag(status.tags, sizeof(status.tags), "FIRST_COMP_ALL_ZERO");
		state.startup_logged_compression_all_zero = true;
	}
}

inline StartupProbeStatus make_startup_probe_status(
	SuspensionDiagnosticsState& state,
	const SuspensionDiagnosticFrame& frame)
{
	StartupProbeStatus status;
	status.total_force = diagnostic_total_force(frame.snapshot);
	status.any_force = status.total_force > frame.config.startup_force_epsilon;
	status.any_wow = diagnostic_any_wow(frame.snapshot);
	status.any_compression = startup_has_compression(frame.snapshot);
	update_startup_force_status(state, status);
	update_startup_wow_status(state, status);
	update_startup_compression_status(state, status);
	if (status.tags[0] == '\0')
	{
		strcpy_s(status.tags, sizeof(status.tags), "-");
	}
	return status;
}

inline void append_startup_environment(
	TextBufferWriter& writer,
	const SuspensionDiagnosticsState& state,
	const SuspensionDiagnosticFrame& frame)
{
	const SuspensionDiagnosticsSnapshot& snapshot = frame.snapshot;
	writer.append(
		"STARTUP_SUSP sim_time=%.4f startup_time=%.4f frame=%llu dt=%.5f "
		"AGL=%.3f h=%.3f h_obj=%.3f vy=%.3f pitch=%.3f roll=%.3f "
		"mass=%.1f gear_down=%d ",
		snapshot.simulation_time, state.startup_probe_elapsed,
		state.startup_frame_index, frame.dt, snapshot.altitude_agl,
		snapshot.surface_height, snapshot.surface_height_with_objects,
		snapshot.vertical_velocity, snapshot.pitch_deg, snapshot.roll_deg,
		snapshot.current_mass, snapshot.gear_pos > 0.5 ? 1 : 0);
}

inline void append_startup_contacts(
	TextBufferWriter& writer,
	const SuspensionDiagnosticsSnapshot& snapshot,
	const StartupProbeStatus& status)
{
	writer.append(
		"wow=%d/%d/%d comp=%.6f/%.6f/%.6f "
		"force_vec=(%.1f,%.1f,%.1f)/(%.1f,%.1f,%.1f)/(%.1f,%.1f,%.1f) "
		"force_mag=%.1f/%.1f/%.1f SUSP_total=%.1f ",
		snapshot.wow[0] ? 1 : 0, snapshot.wow[1] ? 1 : 0,
		snapshot.wow[2] ? 1 : 0, snapshot.compression[0],
		snapshot.compression[1], snapshot.compression[2],
		snapshot.force_vec[0].x, snapshot.force_vec[0].y,
		snapshot.force_vec[0].z, snapshot.force_vec[1].x,
		snapshot.force_vec[1].y, snapshot.force_vec[1].z,
		snapshot.force_vec[2].x, snapshot.force_vec[2].y,
		snapshot.force_vec[2].z, snapshot.force_magnitude[0],
		snapshot.force_magnitude[1], snapshot.force_magnitude[2],
		status.total_force);
}

inline void append_startup_motion(
	TextBufferWriter& writer,
	const SuspensionDiagnosticsSnapshot& snapshot,
	const StartupProbeStatus& status)
{
	writer.append(
		"throttle_output=%.3f/%.3f thrust=%.1f/%.1f "
		"vel_world=(%.3f,%.3f,%.3f) vel_body=(%.3f,%.3f,%.3f) "
		"omega_world=(%.5f,%.5f,%.5f) omega_body=(%.5f,%.5f,%.5f) tags=%s",
		snapshot.left_throttle_output, snapshot.right_throttle_output,
		snapshot.left_thrust, snapshot.right_thrust, snapshot.velocity_world.x,
		snapshot.velocity_world.y, snapshot.velocity_world.z,
		snapshot.velocity_body.x, snapshot.velocity_body.y, snapshot.velocity_body.z,
		snapshot.angular_velocity_world.x, snapshot.angular_velocity_world.y,
		snapshot.angular_velocity_world.z, snapshot.angular_velocity_body.x,
		snapshot.angular_velocity_body.y, snapshot.angular_velocity_body.z,
		status.tags);
}

template <typename ProbeSink>
inline void log_startup_suspension_probe(
	SuspensionDiagnosticsState& state,
	const SuspensionDiagnosticFrame& frame,
	ProbeSink probe_log)
{
	if (!frame.config.startup_probe_enabled ||
		state.startup_probe_elapsed > frame.config.startup_probe_duration)
	{
		return;
	}
	++state.startup_frame_index;
	const StartupProbeStatus status = make_startup_probe_status(state, frame);
	char buffer[2048];
	TextBufferWriter writer(buffer, sizeof(buffer));
	append_startup_environment(writer, state, frame);
	append_startup_contacts(writer, frame.snapshot, status);
	append_startup_motion(writer, frame.snapshot, status);
	probe_log(buffer);
	if (frame.dt > 0.0)
	{
		state.startup_probe_elapsed += frame.dt;
	}
}

inline void append_periodic_probe_header(
	TextBufferWriter& writer,
	const SuspensionDiagnosticFrame& frame)
{
	const SuspensionDiagnosticsConfig& config = frame.config;
	const SuspensionDiagnosticsSnapshot& snapshot = frame.snapshot;
	writer.append(
		"SUSP_PROBE mode=1 SUSP_TEST_MARK=%s geometry_test=%d radius_add=%.2f "
		"wheel_y_offset=%.2f AGL=%.3f gear_down=%d nodes=%s/%s/%s ",
		config.test_mark, config.geometry_test ? 1 : 0, config.radius_add,
		config.wheel_y_offset, snapshot.altitude_agl,
		snapshot.gear_pos > 0.5 ? 1 : 0, config.wheel_nodes[0],
		config.wheel_nodes[1], config.wheel_nodes[2]);
}

inline void append_periodic_probe_state(
	TextBufferWriter& writer,
	const SuspensionDiagnosticsSnapshot& snapshot)
{
	writer.append(
		"valid=%d/%d/%d WOW=%d/%d/%d compression=%.5f/%.5f/%.5f "
		"force=%.1f/%.1f/%.1f brake_moment=%.3f/%.3f/%.3f "
		"input_only_raw_brake=%.3f/%.3f/%.3f input_only_raw_yaw=%.3f "
		"rudder_command=%.3f NWS_command=%.3f NWS_drawarg=%.3f",
		snapshot.feedback_valid[0] ? 1 : 0, snapshot.feedback_valid[1] ? 1 : 0,
		snapshot.feedback_valid[2] ? 1 : 0, snapshot.wow[0] ? 1 : 0,
		snapshot.wow[1] ? 1 : 0, snapshot.wow[2] ? 1 : 0,
		snapshot.compression[0], snapshot.compression[1], snapshot.compression[2],
		snapshot.force_magnitude[0], snapshot.force_magnitude[1],
		snapshot.force_magnitude[2], 0.0,
		Common::limit(snapshot.brake_left, 0.0, 1.0),
		Common::limit(snapshot.brake_right, 0.0, 1.0), snapshot.brake,
		snapshot.brake_left, snapshot.brake_right, snapshot.yaw_input,
		snapshot.rudder_command, snapshot.nose_wheel_command,
		snapshot.nose_wheel_draw_arg);
}

template <typename ProbeSink>
inline void update_periodic_suspension_probe(
	SuspensionDiagnosticsState& state,
	const SuspensionDiagnosticFrame& frame,
	ProbeSink probe_log)
{
	if (!frame.config.suspension_probe_enabled)
	{
		return;
	}
	state.suspension_probe_timer += frame.dt;
	if (state.suspension_probe_timer < frame.config.suspension_probe_interval)
	{
		return;
	}
	state.suspension_probe_timer = 0.0;
	char buffer[1536];
	TextBufferWriter writer(buffer, sizeof(buffer));
	append_periodic_probe_header(writer, frame);
	append_periodic_probe_state(writer, frame.snapshot);
	probe_log(buffer);
}

inline void append_ground_state(
	TextBufferWriter& writer,
	const SuspensionDiagnosticFrame& frame)
{
	const SuspensionDiagnosticsConfig& config = frame.config;
	const SuspensionDiagnosticsSnapshot& snapshot = frame.snapshot;
	writer.append(
		"ground agl=%.3f h=%.3f h_obj=%.3f surf=%u vy=%.3f gear=%.2f mass=%.1f "
		"native=(valid:%d wow:%d comp:%.4f/%.4f/%.4f force:%.1f/%.1f/%.1f) "
		"fallback=(enabled:%d wow:%d fg:%.1f comp:%.4f/%.4f/%.4f) "
		"pitch=%.2f roll=%.2f nws=%d yaw=%.2f nwsarg=%.2f brk=%.2f/%.2f",
		snapshot.altitude_agl, snapshot.surface_height,
		snapshot.surface_height_with_objects, snapshot.surface_type,
		snapshot.vertical_velocity, snapshot.gear_pos, snapshot.current_mass,
		diagnostic_any_feedback(snapshot) ? 1 : 0,
		diagnostic_any_wow(snapshot) ? 1 : 0, snapshot.compression[0],
		snapshot.compression[1], snapshot.compression[2],
		snapshot.force_magnitude[0], snapshot.force_magnitude[1],
		snapshot.force_magnitude[2], config.fallback_enabled ? 1 : 0,
		diagnostic_any_fallback_wow(snapshot) ? 1 : 0,
		snapshot.fallback_ground_force, snapshot.fallback_compression[0],
		snapshot.fallback_compression[1], snapshot.fallback_compression[2],
		snapshot.pitch_deg, snapshot.roll_deg,
		snapshot.nose_turn_enabled ? 1 : 0, snapshot.yaw_input,
		snapshot.nose_wheel_draw_arg, snapshot.brake_left, snapshot.brake_right);
}

template <typename DebugSink>
inline void update_periodic_ground_log(
	SuspensionDiagnosticsState& state,
	const SuspensionDiagnosticFrame& frame,
	DebugSink debug_log)
{
	if (++state.fallback_log_decimation < frame.config.ground_log_decimation)
	{
		return;
	}
	char buffer[768];
	TextBufferWriter writer(buffer, sizeof(buffer));
	append_ground_state(writer, frame);
	debug_log(buffer);
	state.fallback_log_decimation = 0;
}
}
