#include "DcsSnapshots.h"

namespace DcsBridge
{
DrawArgState make_draw_arg_state(const Core::FrameOutput& output)
{
	return {
		output.landing_gear.gear_position,
		output.landing_gear.nose_wheel_steering,
		output.controls.elevator_command,
		output.controls.flaps_position,
		output.controls.aileron_command,
		output.controls.rudder_command,
		output.controls.airbrake_position,
		output.engines[0].afterburner_ratio,
		output.engines[1].afterburner_ratio,
		output.engines[1].nozzle_aperture,
		output.engines[0].nozzle_aperture,
		output.controls.slats_position,
		{
			output.landing_gear.wheel_spin[0],
			output.landing_gear.wheel_spin[1],
			output.landing_gear.wheel_spin[2]
		}
	};
}

ParamExportState make_param_export_state(const Core::FrameOutput& output)
{
	const bool suspension_available =
		output.availability.suspension[0] ||
		output.availability.suspension[1] ||
		output.availability.suspension[2];
	return {
		suspension_available,
		output.suspension.any_weight_on_wheels,
		output.landing_gear.gear_position,
		output.landing_gear.nose_wheel_steering,
		{
			output.landing_gear.wheel_spin[0],
			output.landing_gear.wheel_spin[1],
			output.landing_gear.wheel_spin[2]
		},
		output.landing_gear.brake_left,
		output.landing_gear.brake_right,
		output.controls.pitch_input,
		output.controls.roll_input,
		output.controls.yaw_input,
		output.engines[0].switch_on,
		output.engines[1].switch_on,
		output.engines[0].throttle_input,
		output.engines[1].throttle_input,
		output.engines[0].throttle_output,
		output.engines[1].throttle_output,
		output.engines[0].power_readout,
		output.engines[1].power_readout,
		output.engines[0].thrust_force,
		output.engines[1].thrust_force,
		output.flight.atmosphere_temperature_k,
		output.fuel.internal_fuel,
		output.fuel.total_fuel
	};
}

}
