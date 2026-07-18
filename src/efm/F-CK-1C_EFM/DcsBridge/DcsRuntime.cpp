#include "DcsRuntime.h"

#include "ConfigReader.h"
#include "DcsSnapshots.h"
#include "../Common/PathUtils.h"
#include "../Diagnostics/DebugLogger.h"

namespace
{
constexpr size_t kModulePathBufferSize = DcsBridge::kModulePathMax;
constexpr double kLegacyCockpitTemperatureOffset = 273.0;
constexpr double kSuspensionTestRadiusAdd = 0.30;
constexpr double kSuspensionTestWheelYOffset = -0.50;
constexpr double kCarrierLaunchEngineShare = 0.5;
constexpr double kCarrierLaunchEngineCount = 2.0;
constexpr const char* kEfmVersion = "v0.1.3-april-fools";
constexpr const char* kEfmVersionDate = "2026-04-01";
constexpr const char* kInvalidSuspensionFeedback =
	"suspension_feedback: invalid idx or null info";

const char* kOriginalWheelNodes[3] = { "WHEEL_F", "WHEEL_L", "WHEEL_R" };
const char* kModelViewerWheelNodes[3] = { "WHEEL_F", "WHEEL_L", "WHEEL_R" };
}

namespace DcsBridge
{
DcsRuntime::DcsRuntime()
	: cockpit_params_(make_cockpit_param_handles(cockpit_interface_)),
	autopilot_params_(make_autopilot_param_handles(cockpit_interface_))
{
}

Core::AutopilotCommand DcsRuntime::read_autopilot()
{
	if (!autopilot_params_available(autopilot_params_))
	{
		reset_autopilot_state(autopilot_state_);
		return {};
	}

	update_autopilot_from_lua(cockpit_interface_, autopilot_params_, autopilot_state_);
	Core::AutopilotCommand command;
	command.master = autopilot_state_.master;
	command.bypass = autopilot_state_.bypass;
	command.auto_throttle_engaged = autopilot_state_.at_engaged;
	command.pitch_command = autopilot_state_.pitch_cmd;
	command.roll_command = autopilot_state_.roll_cmd;
	command.throttle_command = autopilot_state_.throttle_cmd;
	return command;
}

Core::MaxPowerCommand DcsRuntime::read_max_power()
{
	const MaxPowerSwitchState state = read_max_power_switch(cockpit_interface_, cockpit_params_);
	Core::MaxPowerCommand command;
	command.ready = state.ready;
	command.value = state.value;
	return command;
}

void DcsRuntime::on_first_frame(const Core::Fck1cEfmSnapshot& snapshot)
{
	refresh_suspension_diagnostics_config(snapshot);
	const double simulation_time = snapshot.systems.startup.simulation_time;
	Diagnostics::log_ground_configuration_once(
		suspension_diagnostics_,
		suspension_diagnostics_config_,
		Diagnostics::make_diagnostic_sinks(
			[this](const char* message) { write_module_log(message); },
			[this, simulation_time](const char* message)
			{
				write_probe_log(simulation_time, message);
			}));
}

void DcsRuntime::on_engine_shutdown(const Core::Fck1cEfmSnapshot& snapshot)
{
	const Core::AircraftState& aircraft = snapshot.aircraft;
	const Core::Fck1cEfmSystems& systems = snapshot.systems;
	char message[256];
	Diagnostics::format_engine_shutdown(
		{ message, sizeof(message) },
		{
			systems.fuel.internal_fuel,
			aircraft.altitude_asl,
			systems.engines.left.switch_on,
			systems.engines.right.switch_on
		});
	write_module_log(message);
}

void DcsRuntime::on_thrust_updated(
	const Core::Fck1cEfmSnapshot& snapshot,
	const Core::MaxPowerCommand& command)
{
	char message[768];
	Diagnostics::format_thrust_diagnostics(
		{ message, sizeof(message) },
		make_thrust_snapshot(snapshot, command));
	write_module_log(message);
}

void DcsRuntime::on_ground_diagnostics(const Core::Fck1cEfmSnapshot& source, double dt)
{
	if (!suspension_diagnostics_config_loaded_)
	{
		refresh_suspension_diagnostics_config(source);
	}
	const Diagnostics::SuspensionDiagnosticsSnapshot snapshot = make_suspension_snapshot(source);
	const double simulation_time = source.systems.startup.simulation_time;
	const Diagnostics::SuspensionDiagnosticFrame frame = {
		suspension_diagnostics_config_, snapshot, dt
	};
	Diagnostics::log_startup_suspension_probe(
		suspension_diagnostics_, frame,
		[this, simulation_time](const char* message) { write_probe_log(simulation_time, message); });
	Diagnostics::update_periodic_suspension_probe(
		suspension_diagnostics_, frame,
		[this, simulation_time](const char* message) { write_probe_log(simulation_time, message); });
	Diagnostics::update_periodic_ground_log(
		suspension_diagnostics_, frame,
		[this](const char* message) { write_module_log(message); });
}

void DcsRuntime::on_release(const Core::Fck1cEfmSnapshot& snapshot)
{
	(void)snapshot;
	reset_autopilot_state(autopilot_state_);
}

void DcsRuntime::configure(
	const char* config_path,
	const Core::Fck1cEfmSnapshot& snapshot)
{
	configure_module_paths(module_paths_, config_path);
	refresh_suspension_diagnostics_config(snapshot);
}

void DcsRuntime::export_temperature(double dcs_temperature)
{
	export_temperature_param(
		cockpit_interface_,
		cockpit_params_,
		dcs_temperature + kLegacyCockpitTemperatureOffset);
}

void DcsRuntime::reset_startup_suspension_probe()
{
	Diagnostics::reset_startup_suspension_probe(suspension_diagnostics_);
}

void DcsRuntime::reset_carrier_launch()
{
	reset_carrier_launch_state(carrier_launch_);
}

void DcsRuntime::log_damage(
	const Core::Fck1cEfmSnapshot& snapshot,
	int element,
	double integrity)
{
	char message[160];
	Diagnostics::format_damage_event(
		{ message, sizeof(message) },
		{ element, integrity, snapshot.gameplay.invincible });
	write_module_log(message);
	write_probe_log(snapshot.systems.startup.simulation_time, message);
}

void DcsRuntime::update_suspension_feedback(
	Core::Fck1cEfm& efm,
	int index,
	const ed_fm_suspension_info* info)
{
	if (info == nullptr)
	{
		write_probe_log(
			efm.snapshot().systems.startup.simulation_time,
			kInvalidSuspensionFeedback);
		write_module_log(kInvalidSuspensionFeedback);
		return;
	}
	const Core::SuspensionFeedbackInput feedback = {
		index,
		Common::Vec3(
			info->acting_force[0],
			info->acting_force[1],
			info->acting_force[2]),
		Common::Vec3(
			info->acting_force_point[0],
			info->acting_force_point[1],
			info->acting_force_point[2]),
		info->integrity_factor,
		info->struct_compression,
		info->wheel_speed_X
	};
	if (!efm.update_suspension_feedback(feedback))
	{
		write_probe_log(
			efm.snapshot().systems.startup.simulation_time,
			kInvalidSuspensionFeedback);
		write_module_log(kInvalidSuspensionFeedback);
		return;
	}
	const Core::Fck1cEfmSnapshot source = efm.snapshot();
	const Systems::SuspensionSystemState& suspension = source.systems.suspension;
	char message[512];
	Diagnostics::format_suspension_feedback(
		{ message, sizeof(message) },
		{
			index,
			suspension.compression[index],
			feedback.acting_force,
			suspension.force_mag[index],
			suspension.wow[index]
		});
	write_module_log(message);
	Diagnostics::format_suspension_animation(
		{ message, sizeof(message) },
		{
			index,
			suspension.compression[index],
			static_cast<float>(source.suspension_visual_arg[index]),
			info->wheel_speed_X
		});
	write_probe_log(source.systems.startup.simulation_time, message);
}

bool DcsRuntime::pop_simulation_event(
	const Core::Fck1cEfmSnapshot& snapshot,
	double carrier_launch_thrust,
	ed_fm_simulation_event& out)
{
	return pop_carrier_launch_event(
		carrier_launch_,
		out,
		{
			snapshot.systems.engines.left.throttle_output,
			carrier_launch_thrust *
				kCarrierLaunchEngineShare * kCarrierLaunchEngineCount
		});
}

bool DcsRuntime::push_simulation_event(const ed_fm_simulation_event& in)
{
	return push_carrier_launch_event(carrier_launch_, in);
}

size_t DcsRuntime::debug_watch(
	const Core::Fck1cEfmSnapshot& snapshot,
	int level,
	const DebugWatchBuffer& buffer) const
{
	return Diagnostics::format_debug_watch(
		level,
		make_debug_watch_snapshot(snapshot, kEfmVersion, kEfmVersionDate),
		{ buffer.data, buffer.capacity });
}

const char* DcsRuntime::active_fm_config_path()
{
	return DcsBridge::active_fm_config_path(module_paths_);
}

void DcsRuntime::build_mod_path(char* output, size_t output_size, const char* relative_path)
{
	DcsBridge::build_mod_path(
		module_paths_, { output, output_size }, relative_path);
}

bool DcsRuntime::config_flag_is_true(const char* flag_name)
{
	return DcsBridge::config_flag_is_true(active_fm_config_path(), flag_name);
}

double DcsRuntime::config_number_or_default(const char* key_name, double default_value)
{
	return DcsBridge::config_number_or_default(
		active_fm_config_path(), key_name, default_value);
}

void DcsRuntime::config_string_or_default(
	const char* key_name,
	const char* default_value,
	const ConfigStringTarget& target)
{
	DcsBridge::config_string_or_default(
		{ active_fm_config_path(), key_name, default_value },
		{ target.data, target.capacity });
}

double DcsRuntime::active_suspension_radius_add()
{
	return config_flag_is_true("SUSP_GEOMETRY_TEST")
		? config_number_or_default("SUSP_GEOMETRY_TEST_RADIUS_ADD", kSuspensionTestRadiusAdd)
		: 0.0;
}

double DcsRuntime::active_suspension_wheel_y_offset()
{
	return config_flag_is_true("SUSP_GEOMETRY_TEST")
		? config_number_or_default(
			"SUSP_GEOMETRY_TEST_WHEEL_Y_OFFSET", kSuspensionTestWheelYOffset)
		: 0.0;
}

void DcsRuntime::active_suspension_node_names(
	const char*& nose,
	const char*& left,
	const char*& right)
{
	const bool use_model_viewer = config_flag_is_true("SUSP_USE_MODELVIEWER_WHEEL_NODES");
	const char** nodes = use_model_viewer ? kModelViewerWheelNodes : kOriginalWheelNodes;
	nose = nodes[0];
	left = nodes[1];
	right = nodes[2];
}

void DcsRuntime::refresh_suspension_diagnostics_config(
	const Core::Fck1cEfmSnapshot& snapshot)
{
	const Core::SuspensionConfigurationSnapshot& suspension =
		snapshot.suspension_configuration;
	Diagnostics::SuspensionDiagnosticsConfig config;
	config.use_modelviewer_nodes = config_flag_is_true("SUSP_USE_MODELVIEWER_WHEEL_NODES");
	config.geometry_test = config_flag_is_true("SUSP_GEOMETRY_TEST");
	config.radius_add = active_suspension_radius_add();
	config.wheel_y_offset = active_suspension_wheel_y_offset();
	config_string_or_default(
		"SUSP_TEST_MARK",
		"SUSP_TEST_MARK_NOT_FOUND",
		{ config.test_mark, sizeof(config.test_mark) });
	active_suspension_node_names(
		config.wheel_nodes[0], config.wheel_nodes[1], config.wheel_nodes[2]);
	for (int index = 0; index < Diagnostics::kDiagnosticWheelCount; ++index)
	{
		config.final_wheel_radius[index] =
			suspension.wheel_radius[index] + config.radius_add;
		config.final_wheel_pos[index] = suspension.wheel_position[index];
		config.final_wheel_pos[index].y += config.wheel_y_offset;
	}
	config.active_collision_shell = suspension.active_collision_shell;
	config.suspension_mode = suspension.mode_name;
	config.fallback_enabled = suspension.fallback_enabled;
	config.build_date = __DATE__;
	config.build_time = __TIME__;
	suspension_diagnostics_config_ = config;
	suspension_diagnostics_config_loaded_ = true;
}

Diagnostics::SuspensionDiagnosticsSnapshot DcsRuntime::make_suspension_snapshot(
	const Core::Fck1cEfmSnapshot& source) const
{
	const Core::AircraftState& aircraft = source.aircraft;
	const Core::Fck1cEfmSystems& systems = source.systems;
	const Core::ControlSurfaceState& controls = source.control_surfaces;
	Diagnostics::SuspensionDiagnosticsSnapshot snapshot;
	snapshot.simulation_time = systems.startup.simulation_time;
	snapshot.altitude_agl = aircraft.altitude_agl;
	snapshot.surface_height = aircraft.surface_height_raw;
	snapshot.surface_height_with_objects = aircraft.surface_height_with_objects;
	snapshot.surface_type = aircraft.surface_type_raw;
	snapshot.vertical_velocity = aircraft.velocity_world.y;
	snapshot.pitch_deg = Common::deg(aircraft.pitch);
	snapshot.roll_deg = Common::deg(aircraft.roll);
	snapshot.current_mass = aircraft.current_mass;
	snapshot.gear_pos = systems.landing_gear.position;
	copy_suspension_wheel_snapshot(systems, snapshot);
	snapshot.fallback_ground_force = systems.suspension.fallback_ground_force;
	snapshot.left_throttle_output = systems.engines.left.throttle_output;
	snapshot.right_throttle_output = systems.engines.right.throttle_output;
	snapshot.left_thrust = systems.engines.left.thrust_force;
	snapshot.right_thrust = systems.engines.right.thrust_force;
	snapshot.velocity_world = aircraft.velocity_world;
	snapshot.velocity_body = aircraft.velocity_body;
	snapshot.angular_velocity_world = aircraft.angular_velocity_world;
	snapshot.angular_velocity_body = aircraft.angular_velocity_body;
	snapshot.brake = systems.landing_gear.wheels.brake;
	snapshot.brake_left = systems.landing_gear.wheels.brake_left;
	snapshot.brake_right = systems.landing_gear.wheels.brake_right;
	snapshot.yaw_input = systems.primary_controls.yaw.input;
	snapshot.rudder_command = controls.rudder_command;
	snapshot.nose_wheel_command = source.nose_wheel_command;
	snapshot.nose_wheel_draw_arg = systems.landing_gear.wheels.nose_steering;
	snapshot.nose_turn_enabled = systems.landing_gear.wheels.nose_turn_enabled;
	return snapshot;
}

void DcsRuntime::copy_suspension_wheel_snapshot(
	const Core::Fck1cEfmSystems& systems,
	Diagnostics::SuspensionDiagnosticsSnapshot& snapshot) const
{
	for (int index = 0; index < Diagnostics::kDiagnosticWheelCount; ++index)
	{
		snapshot.feedback_valid[index] = systems.suspension.feedback_valid[index];
		snapshot.wow[index] = systems.suspension.wow[index];
		snapshot.compression[index] = systems.suspension.compression[index];
		snapshot.force_vec[index] = systems.suspension.force_vec[index];
		snapshot.force_magnitude[index] = systems.suspension.force_mag[index];
		snapshot.fallback_wow[index] = systems.suspension.fallback_wow[index];
		snapshot.fallback_compression[index] = systems.suspension.fallback_compression[index];
	}
}

Diagnostics::ThrustDiagnosticsSnapshot DcsRuntime::make_thrust_snapshot(
	const Core::Fck1cEfmSnapshot& source,
	const Core::MaxPowerCommand& command) const
{
	const Core::Fck1cEfmSystems& systems = source.systems;
	const Core::ForceMomentFrame& frame = source.force_moment;
	Diagnostics::ThrustDiagnosticsSnapshot snapshot;
	snapshot.left_thrust = systems.engines.left.thrust_force;
	snapshot.right_thrust = systems.engines.right.thrust_force;
	const Common::Vec3 left_force(snapshot.left_thrust, 0.0, 0.0);
	const Common::Vec3 right_force(snapshot.right_thrust, 0.0, 0.0);
	const Common::Vec3 left_moment = Common::cross(source.left_engine_position, left_force);
	const Common::Vec3 right_moment = Common::cross(source.right_engine_position, right_force);
	snapshot.net_moment = Common::Vec3(
		left_moment.x + right_moment.x + frame.moment.x,
		left_moment.y + right_moment.y + frame.moment.y,
		left_moment.z + right_moment.z + frame.moment.z);
	for (int index = 0; index < Diagnostics::kDiagnosticWheelCount; ++index)
	{
		snapshot.suspension_force[index] = systems.suspension.force_mag[index];
	}
	snapshot.maxpower_ready = command.ready;
	snapshot.maxpower_value = command.value;
	snapshot.left_engine_switch = systems.engines.left.switch_on;
	snapshot.right_engine_switch = systems.engines.right.switch_on;
	snapshot.left_throttle_input = systems.engines.left.throttle_input;
	snapshot.right_throttle_input = systems.engines.right.throttle_input;
	snapshot.left_throttle_output = systems.engines.left.throttle_output;
	snapshot.right_throttle_output = systems.engines.right.throttle_output;
	snapshot.left_power_readout = systems.engines.left.power_readout;
	snapshot.right_power_readout = systems.engines.right.power_readout;
	snapshot.left_wing_integrity = systems.damage.left_wing_integrity;
	snapshot.right_wing_integrity = systems.damage.right_wing_integrity;
	snapshot.left_engine_integrity = systems.damage.left_engine_integrity;
	snapshot.right_engine_integrity = systems.damage.right_engine_integrity;
	snapshot.internal_fuel = systems.fuel.internal_fuel;
	return snapshot;
}

void DcsRuntime::write_module_log(const char* message)
{
	char debug_directory[kModulePathBufferSize];
	build_mod_path(debug_directory, sizeof(debug_directory), "debug");
	Diagnostics::write_module_debug_log(debug_directory, message);
}

void DcsRuntime::write_probe_log(double simulation_time, const char* message)
{
	char log_directory[1024];
	if (!Common::resolve_saved_games_logs_dir(log_directory, sizeof(log_directory)))
	{
		build_mod_path(log_directory, sizeof(log_directory), "debug");
	}
	Diagnostics::write_suspension_probe_log(
		log_directory,
		simulation_time,
		message);
}
}
