#include "DcsRuntime.h"

#include "Internal/FrameInputCollector.h"

namespace
{
constexpr double kLegacyCockpitTemperatureOffset = 273.0;
constexpr double kCarrierLaunchEngineShare = 0.5;
constexpr double kCarrierLaunchEngineCount = 2.0;
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

bool DcsRuntime::publish_suspension_feedback(
	Internal::FrameInputCollector& collector,
	int index,
	const ed_fm_suspension_info* info)
{
	if (info == nullptr)
	{
		return false;
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
	return collector.publish_suspension(feedback);
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

}
