#pragma once

#include "../../Common/Clamp.h"
#include "../../Core/FrameContracts.h"
#include "../../DcsIds/DrawArgs.h"
#include "../../include/FM/wHumanCustomPhysicsAPI.h"
#include <stddef.h>

namespace DcsBridge
{
struct DrawArgState
{
	double gear_pos;
	double nose_wheel_steering;
	double elevator_command;
	double flaps_pos;
	double aileron_command;
	double rudder_command;
	double airbrake_pos;
	double left_afterburner_ratio;
	double right_afterburner_ratio;
	double right_nozzle_aperture;
	double left_nozzle_aperture;
	double slats_pos;
	double wheel_spin[3];
};

inline DrawArgState make_draw_arg_state(const Core::FrameOutput& output)
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

inline void set_draw_args(EdDrawArgument* drawargs, size_t size, const DrawArgState& state)
{
	(void)size;

	// Landing gear
	drawargs[DcsIds::DrawArgs::NoseGear].f = (float)Common::limit(state.gear_pos, 0, 1);
	drawargs[DcsIds::DrawArgs::RightGear].f = (float)Common::limit(state.gear_pos, 0, 1);
	drawargs[DcsIds::DrawArgs::LeftGear].f = (float)Common::limit(state.gear_pos, 0, 1);
	drawargs[DcsIds::DrawArgs::NoseWheelSteering].f = (float)Common::limit(state.nose_wheel_steering, -1, 1);

	// Elevators/stabilators
	drawargs[DcsIds::DrawArgs::LeftElevator].f = (float)Common::limit(state.elevator_command, -1, 1);
	drawargs[DcsIds::DrawArgs::RightElevator].f = (float)Common::limit(state.elevator_command, -1, 1);

	// On this model, flap-related trailing-edge surfaces use negative drawarg
	// values for trailing-edge-down deflection. Keep the aerodynamic flap state
	// positive in FM logic and only invert the visual mapping here.
	const double flap_visual = -Common::limit(state.flaps_pos, 0.0, 1.0);
	drawargs[DcsIds::DrawArgs::RightFlaperon].f = (float)Common::limit(flap_visual + state.aileron_command, -1, 1);
	drawargs[DcsIds::DrawArgs::LeftFlaperon].f = (float)Common::limit(flap_visual - state.aileron_command, -1, 1);

	// Rudder(s)
	drawargs[DcsIds::DrawArgs::RudderPrimary].f = (float)Common::limit(state.rudder_command, -1, 1);
	drawargs[DcsIds::DrawArgs::RudderSecondary].f = (float)Common::limit(state.rudder_command, -1, 1);

	// Airbrake(s)
	drawargs[DcsIds::DrawArgs::AirbrakePrimary].f = (float)Common::limit(state.airbrake_pos, 0, 1);
	drawargs[DcsIds::DrawArgs::AirbrakeSecondary].f = (float)Common::limit(state.airbrake_pos, 0, 1);
	drawargs[DcsIds::DrawArgs::AirbrakeTertiary].f = (float)Common::limit(state.airbrake_pos, 0, 1);

	// Afterburner intensity
	drawargs[DcsIds::DrawArgs::LeftAfterburner].f = (float)Common::limit(state.left_afterburner_ratio, 0, 1);
	drawargs[DcsIds::DrawArgs::RightAfterburner].f = (float)Common::limit(state.right_afterburner_ratio, 0, 1);

	// Nozzle aperture: MV2-confirmed mapping is 89 = right engine, 90 = left engine.
	drawargs[DcsIds::DrawArgs::RightNozzle].f = (float)Common::limit(state.right_nozzle_aperture, 0, 1);
	drawargs[DcsIds::DrawArgs::LeftNozzle].f = (float)Common::limit(state.left_nozzle_aperture, 0, 1);

	// Practical model mapping based on in-sim verification:
	// 9/10 behave like the leading-edge slot pieces, while 11/12 are the flaperons.
	const double slat_visual = Common::limit(state.slats_pos, 0.0, 1.0);
	drawargs[DcsIds::DrawArgs::LeftSlat].f = (float)slat_visual;
	drawargs[DcsIds::DrawArgs::RightSlat].f = (float)slat_visual;

	// Wheel spin bones: 76 = nose, 101 = left main, 102 = right main.
	drawargs[DcsIds::DrawArgs::NoseWheelSpin].f = (float)state.wheel_spin[0];
	drawargs[DcsIds::DrawArgs::LeftWheelSpin].f = (float)state.wheel_spin[1];
	drawargs[DcsIds::DrawArgs::RightWheelSpin].f = (float)state.wheel_spin[2];
}
}
