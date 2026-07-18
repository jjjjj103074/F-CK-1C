#include "DcsRuntime.h"

#include "../Diagnostics/DebugLogger.h"
#include "../Diagnostics/RuntimeDiagnostics.h"

namespace
{
constexpr size_t kModulePathBufferSize = DcsBridge::kModulePathMax;
constexpr double kLegacyCockpitTemperatureOffset = 273.0;
constexpr double kCarrierLaunchEngineShare = 0.5;
constexpr double kCarrierLaunchEngineCount = 2.0;
constexpr const char* kInvalidSuspensionFeedback =
	"suspension_feedback: invalid idx or null info";
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

void DcsRuntime::configure(const char* config_path)
{
	configure_module_paths(module_paths_, config_path);
}

void DcsRuntime::export_temperature(double dcs_temperature)
{
	export_temperature_param(
		cockpit_interface_,
		cockpit_params_,
		dcs_temperature + kLegacyCockpitTemperatureOffset);
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
}

void DcsRuntime::update_suspension_feedback(
	Core::Fck1cEfm& efm,
	int index,
	const ed_fm_suspension_info* info)
{
	if (info == nullptr)
	{
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
		write_module_log(kInvalidSuspensionFeedback);
	}
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

void DcsRuntime::build_mod_path(char* output, size_t output_size, const char* relative_path)
{
	DcsBridge::build_mod_path(
		module_paths_, { output, output_size }, relative_path);
}

void DcsRuntime::write_module_log(const char* message)
{
	char debug_directory[kModulePathBufferSize];
	build_mod_path(debug_directory, sizeof(debug_directory), "debug");
	Diagnostics::write_module_debug_log(debug_directory, message);
}
}
