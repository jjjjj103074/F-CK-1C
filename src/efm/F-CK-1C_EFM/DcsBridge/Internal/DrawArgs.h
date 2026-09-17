#pragma once

#include "../../Common/Clamp.h"
#include "../../Common/Units.h"
#include "../../Core/Contracts/FrameContracts.h"
#include "../../DcsIds/DrawArgs.h"
#include "../../include/FM/wHumanCustomPhysicsAPI.h"
#include <stddef.h>

namespace DcsBridge
{
inline constexpr double kStabilatorVisualTravelRad = Common::rad(25.0);
inline constexpr double kFlaperonVisualTravelRad = Common::rad(22.0);
inline constexpr double kRudderVisualTravelRad = Common::rad(30.0);

struct DrawArgState
{
	double gear_position_normalized;
	double nose_wheel_steering_normalized;
	double elevator_command_normalized;
	double flaps_position_normalized;
	double aileron_command_normalized;
	double rudder_command_normalized;
	double airbrake_position_normalized;
	double left_afterburner_ratio_0_1;
	double right_afterburner_ratio_0_1;
	double right_nozzle_aperture_normalized;
	double left_nozzle_aperture_normalized;
	double slats_position_normalized;
	double wheel_spin_phase_0_1[3];
};

inline DrawArgState make_draw_arg_state(const Core::FrameOutput& output)
{
	return {
		output.landing_gear.gear_position_normalized,
		output.landing_gear.nose_wheel_steering_normalized,
		output.controls.symmetric_stabilator_position_rad /
			kStabilatorVisualTravelRad,
		output.controls.flaps_position_normalized,
		output.controls.differential_flaperon_position_rad /
			kFlaperonVisualTravelRad,
		output.controls.rudder_position_rad / kRudderVisualTravelRad,
		output.controls.airbrake_position_normalized,
		output.engines[0].afterburner_ratio_0_1,
		output.engines[1].afterburner_ratio_0_1,
		output.engines[1].nozzle_aperture_normalized,
		output.engines[0].nozzle_aperture_normalized,
		output.controls.slats_position_normalized,
		{
			output.landing_gear.wheel_spin_phase_0_1[0],
			output.landing_gear.wheel_spin_phase_0_1[1],
			output.landing_gear.wheel_spin_phase_0_1[2]
		}
	};
}

inline void set_gear_draw_args(
	EdDrawArgument* drawargs,
	const DrawArgState& state)
{
	drawargs[DcsIds::DrawArgs::NoseGear].f =
		(float)Common::limit(state.gear_position_normalized, 0, 1);
	drawargs[DcsIds::DrawArgs::RightGear].f =
		(float)Common::limit(state.gear_position_normalized, 0, 1);
	drawargs[DcsIds::DrawArgs::LeftGear].f =
		(float)Common::limit(state.gear_position_normalized, 0, 1);
	drawargs[DcsIds::DrawArgs::NoseWheelSteering].f =
		(float)Common::limit(state.nose_wheel_steering_normalized, -1, 1);
}

inline void set_elevator_draw_args(
	EdDrawArgument* drawargs,
	const DrawArgState& state)
{
	drawargs[DcsIds::DrawArgs::LeftElevator].f =
		(float)Common::limit(state.elevator_command_normalized, -1, 1);
	drawargs[DcsIds::DrawArgs::RightElevator].f =
		(float)Common::limit(state.elevator_command_normalized, -1, 1);
}

inline void set_flaperon_draw_args(
	EdDrawArgument* drawargs,
	const DrawArgState& state)
{
	// On this model, flap-related trailing-edge surfaces use negative drawarg
	// values for trailing-edge-down deflection. Keep the aerodynamic flap state
	// positive in FM logic and only invert the visual mapping here.
	const double flap_visual =
		-Common::limit(state.flaps_position_normalized, 0.0, 1.0);
	drawargs[DcsIds::DrawArgs::RightFlaperon].f = (float)Common::limit(
		flap_visual + state.aileron_command_normalized, -1, 1);
	drawargs[DcsIds::DrawArgs::LeftFlaperon].f = (float)Common::limit(
		flap_visual - state.aileron_command_normalized, -1, 1);
}

inline void set_rudder_draw_args(
	EdDrawArgument* drawargs,
	const DrawArgState& state)
{
	drawargs[DcsIds::DrawArgs::RudderPrimary].f =
		(float)Common::limit(state.rudder_command_normalized, -1, 1);
	drawargs[DcsIds::DrawArgs::RudderSecondary].f =
		(float)Common::limit(state.rudder_command_normalized, -1, 1);
}

inline void set_airbrake_draw_args(
	EdDrawArgument* drawargs,
	const DrawArgState& state)
{
	drawargs[DcsIds::DrawArgs::AirbrakePrimary].f =
		(float)Common::limit(state.airbrake_position_normalized, 0, 1);
	drawargs[DcsIds::DrawArgs::AirbrakeSecondary].f =
		(float)Common::limit(state.airbrake_position_normalized, 0, 1);
	drawargs[DcsIds::DrawArgs::AirbrakeTertiary].f =
		(float)Common::limit(state.airbrake_position_normalized, 0, 1);
}

inline void set_engine_draw_args(
	EdDrawArgument* drawargs,
	const DrawArgState& state)
{
	drawargs[DcsIds::DrawArgs::LeftAfterburner].f =
		(float)Common::limit(state.left_afterburner_ratio_0_1, 0, 1);
	drawargs[DcsIds::DrawArgs::RightAfterburner].f =
		(float)Common::limit(state.right_afterburner_ratio_0_1, 0, 1);

	// Nozzle aperture: MV2-confirmed mapping is 89 = right engine, 90 = left engine.
	drawargs[DcsIds::DrawArgs::RightNozzle].f =
		(float)Common::limit(state.right_nozzle_aperture_normalized, 0, 1);
	drawargs[DcsIds::DrawArgs::LeftNozzle].f =
		(float)Common::limit(state.left_nozzle_aperture_normalized, 0, 1);
}

inline void set_slat_draw_args(
	EdDrawArgument* drawargs,
	const DrawArgState& state)
{
	// Practical model mapping based on in-sim verification:
	// 9/10 behave like the leading-edge slot pieces, while 11/12 are the flaperons.
	const double slat_visual =
		Common::limit(state.slats_position_normalized, 0.0, 1.0);
	drawargs[DcsIds::DrawArgs::LeftSlat].f = (float)slat_visual;
	drawargs[DcsIds::DrawArgs::RightSlat].f = (float)slat_visual;
}

inline void set_wheel_draw_args(
	EdDrawArgument* drawargs,
	const DrawArgState& state)
{
	drawargs[DcsIds::DrawArgs::NoseWheelSpin].f =
		(float)state.wheel_spin_phase_0_1[0];
	drawargs[DcsIds::DrawArgs::LeftWheelSpin].f =
		(float)state.wheel_spin_phase_0_1[1];
	drawargs[DcsIds::DrawArgs::RightWheelSpin].f =
		(float)state.wheel_spin_phase_0_1[2];
}

inline void set_draw_args(
	EdDrawArgument* drawargs,
	size_t size,
	const DrawArgState& state)
{
	(void)size;
	set_gear_draw_args(drawargs, state);
	set_elevator_draw_args(drawargs, state);
	set_flaperon_draw_args(drawargs, state);
	set_rudder_draw_args(drawargs, state);
	set_airbrake_draw_args(drawargs, state);
	set_engine_draw_args(drawargs, state);
	set_slat_draw_args(drawargs, state);
	set_wheel_draw_args(drawargs, state);
}
}
