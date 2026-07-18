#include "DcsSnapshots.h"

namespace DcsBridge
{
DrawArgState make_draw_arg_state(const Core::Fck1cEfmSnapshot& snapshot)
{
	const Core::Fck1cEfmSystems& systems = snapshot.systems;
	const Core::ControlSurfaceState& controls = snapshot.control_surfaces;
	return {
		systems.landing_gear.position,
		systems.landing_gear.wheels.nose_steering,
		controls.elevator_command,
		systems.airframe_devices.flaps_pos,
		controls.aileron_command,
		controls.rudder_command,
		systems.airframe_devices.airbrake_pos,
		systems.engines.left.afterburner_ratio,
		systems.engines.right.afterburner_ratio,
		systems.engines.right.nozzle_aperture,
		systems.engines.left.nozzle_aperture,
		systems.airframe_devices.slats_pos,
		{
			systems.landing_gear.wheels.spin[0],
			systems.landing_gear.wheels.spin[1],
			systems.landing_gear.wheels.spin[2]
		}
	};
}

ParamExportState make_param_export_state(const Core::Fck1cEfmSnapshot& snapshot)
{
	const Core::AircraftState& aircraft = snapshot.aircraft;
	const Core::Fck1cEfmSystems& systems = snapshot.systems;
	return {
		snapshot.suspension_feedback_available,
		snapshot.any_weight_on_wheels,
		systems.landing_gear.position,
		systems.landing_gear.wheels.nose_steering,
		{
			systems.landing_gear.wheels.spin[0],
			systems.landing_gear.wheels.spin[1],
			systems.landing_gear.wheels.spin[2]
		},
		systems.landing_gear.wheels.brake_left,
		systems.landing_gear.wheels.brake_right,
		systems.primary_controls.pitch.input,
		systems.primary_controls.roll.input,
		systems.primary_controls.yaw.input,
		systems.engines.left.switch_on,
		systems.engines.right.switch_on,
		systems.engines.left.throttle_input,
		systems.engines.right.throttle_input,
		systems.engines.left.throttle_output,
		systems.engines.right.throttle_output,
		systems.engines.left.power_readout,
		systems.engines.right.power_readout,
		systems.engines.left.thrust_force,
		systems.engines.right.thrust_force,
		aircraft.atmosphere_temperature,
		systems.fuel.internal_fuel,
		systems.fuel.total_fuel
	};
}

}
