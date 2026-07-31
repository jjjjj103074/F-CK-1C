#include "TestHarness.h"

#include "DcsBridge/Internal/DrawArgs.h"
#include "DcsBridge/Internal/ParamExport.h"

#include <array>

namespace
{
constexpr double kTolerance = 1e-9;
constexpr double kDrawArgTolerance = 1e-6;
constexpr std::size_t kDrawArgumentCount =
	DcsIds::DrawArgs::AirbrakeTertiary + 1;

using DrawArgumentBuffer =
	std::array<EdDrawArgument, kDrawArgumentCount>;

Core::FrameOutput make_frame_output()
{
	Core::FrameOutput output;
	output.availability.suspension = { true, false, false };
	output.availability.atmosphere = true;
	output.landing_gear.gear_position = 0.8;
	output.landing_gear.nose_wheel_steering = -0.25;
	output.landing_gear.brake_left = 0.35;
	output.landing_gear.brake_right = 0.45;
	output.landing_gear.wheel_spin = { 1.0, 2.0, 3.0 };
	output.controls = { 0.11, 0.12, 0.13, 0.2, -0.1, 0.3, 0.4, 0.5, 0.6 };
	output.engines[0] = { true, 0.7, 0.71, 0.72, 12000.0, 0.73, true, 0.74 };
	output.engines[1] = { false, 0.8, 0.81, 0.82, 13000.0, 0.83, false, 0.84 };
	output.flight.atmosphere_temperature_k = 288.0;
	output.suspension.any_weight_on_wheels = true;
	output.fuel = { 900.0, 200.0, 1100.0, 1.5 };
	return output;
}

void test_draw_arg_projection(Tests::Context& context)
{
	const DcsBridge::DrawArgState state =
		DcsBridge::make_draw_arg_state(make_frame_output());
	TEST_EXPECT_NEAR(context, state.gear_pos, 0.8, kTolerance);
	TEST_EXPECT_NEAR(context, state.nose_wheel_steering, -0.25, kTolerance);
	TEST_EXPECT_NEAR(context, state.elevator_command, 0.2, kTolerance);
	TEST_EXPECT_NEAR(context, state.flaps_pos, 0.4, kTolerance);
	TEST_EXPECT_NEAR(context, state.aileron_command, -0.1, kTolerance);
	TEST_EXPECT_NEAR(context, state.rudder_command, 0.3, kTolerance);
	TEST_EXPECT_NEAR(context, state.airbrake_pos, 0.6, kTolerance);
	TEST_EXPECT_NEAR(context, state.left_afterburner_ratio, 0.73, kTolerance);
	TEST_EXPECT_NEAR(context, state.right_afterburner_ratio, 0.83, kTolerance);
	TEST_EXPECT_NEAR(context, state.left_nozzle_aperture, 0.74, kTolerance);
	TEST_EXPECT_NEAR(context, state.right_nozzle_aperture, 0.84, kTolerance);
	TEST_EXPECT_NEAR(context, state.slats_pos, 0.5, kTolerance);
	TEST_EXPECT_NEAR(context, state.wheel_spin[0], 1.0, kTolerance);
	TEST_EXPECT_NEAR(context, state.wheel_spin[1], 2.0, kTolerance);
	TEST_EXPECT_NEAR(context, state.wheel_spin[2], 3.0, kTolerance);
}

DrawArgumentBuffer apply_draw_args(const DcsBridge::DrawArgState& state)
{
	DrawArgumentBuffer draw_args = {};
	DcsBridge::set_draw_args(draw_args.data(), draw_args.size(), state);
	return draw_args;
}

void expect_draw_arg(
	Tests::Context& context,
	const DrawArgumentBuffer& draw_args,
	std::size_t index,
	double expected)
{
	TEST_EXPECT_NEAR(
		context, draw_args[index].f, expected, kDrawArgTolerance);
}

void test_landing_gear_draw_args(Tests::Context& context)
{
	DcsBridge::DrawArgState state = {};
	state.gear_pos = 0.75;
	state.nose_wheel_steering = -0.4;
	const DrawArgumentBuffer draw_args = apply_draw_args(state);
	expect_draw_arg(context, draw_args, DcsIds::DrawArgs::NoseGear, 0.75);
	expect_draw_arg(context, draw_args, DcsIds::DrawArgs::RightGear, 0.75);
	expect_draw_arg(context, draw_args, DcsIds::DrawArgs::LeftGear, 0.75);
	expect_draw_arg(
		context, draw_args, DcsIds::DrawArgs::NoseWheelSteering, -0.4);
}

void expect_rudder_pair(
	Tests::Context& context,
	double command,
	double expected)
{
	DcsBridge::DrawArgState state = {};
	state.rudder_command = command;
	const DrawArgumentBuffer draw_args = apply_draw_args(state);
	expect_draw_arg(
		context, draw_args, DcsIds::DrawArgs::RudderPrimary, expected);
	expect_draw_arg(
		context, draw_args, DcsIds::DrawArgs::RudderSecondary, expected);
}

void test_rudder_draw_args(Tests::Context& context)
{
	expect_rudder_pair(context, -1.0, -1.0);
	expect_rudder_pair(context, 0.0, 0.0);
	expect_rudder_pair(context, 1.0, 1.0);
	expect_rudder_pair(context, -2.0, -1.0);
	expect_rudder_pair(context, 2.0, 1.0);
}

void test_primary_control_surface_draw_args(Tests::Context& context)
{
	DcsBridge::DrawArgState state = {};
	state.elevator_command = 0.6;
	state.flaps_pos = 0.4;
	state.aileron_command = -0.1;
	const DrawArgumentBuffer draw_args = apply_draw_args(state);
	expect_draw_arg(
		context, draw_args, DcsIds::DrawArgs::LeftElevator, 0.6);
	expect_draw_arg(
		context, draw_args, DcsIds::DrawArgs::RightElevator, 0.6);
	expect_draw_arg(
		context, draw_args, DcsIds::DrawArgs::RightFlaperon, -0.5);
	expect_draw_arg(
		context, draw_args, DcsIds::DrawArgs::LeftFlaperon, -0.3);
}

void test_opposed_flaperon_directions(Tests::Context& context)
{
	DcsBridge::DrawArgState state = {};
	state.aileron_command = 1.0;
	DrawArgumentBuffer draw_args = apply_draw_args(state);
	expect_draw_arg(
		context, draw_args, DcsIds::DrawArgs::RightFlaperon, 1.0);
	expect_draw_arg(
		context, draw_args, DcsIds::DrawArgs::LeftFlaperon, -1.0);
	state.aileron_command = -1.0;
	draw_args = apply_draw_args(state);
	expect_draw_arg(
		context, draw_args, DcsIds::DrawArgs::RightFlaperon, -1.0);
	expect_draw_arg(
		context, draw_args, DcsIds::DrawArgs::LeftFlaperon, 1.0);
}

void test_secondary_surface_draw_args(Tests::Context& context)
{
	DcsBridge::DrawArgState state = {};
	state.airbrake_pos = 0.7;
	state.slats_pos = 0.8;
	const DrawArgumentBuffer draw_args = apply_draw_args(state);
	expect_draw_arg(
		context, draw_args, DcsIds::DrawArgs::AirbrakePrimary, 0.7);
	expect_draw_arg(
		context, draw_args, DcsIds::DrawArgs::AirbrakeSecondary, 0.7);
	expect_draw_arg(
		context, draw_args, DcsIds::DrawArgs::AirbrakeTertiary, 0.7);
	expect_draw_arg(context, draw_args, DcsIds::DrawArgs::LeftSlat, 0.8);
	expect_draw_arg(context, draw_args, DcsIds::DrawArgs::RightSlat, 0.8);
}

void test_engine_draw_args(Tests::Context& context)
{
	DcsBridge::DrawArgState state = {};
	state.left_afterburner_ratio = 0.25;
	state.right_afterburner_ratio = 0.75;
	state.left_nozzle_aperture = 0.2;
	state.right_nozzle_aperture = 0.9;
	const DrawArgumentBuffer draw_args = apply_draw_args(state);
	expect_draw_arg(
		context, draw_args, DcsIds::DrawArgs::LeftAfterburner, 0.25);
	expect_draw_arg(
		context, draw_args, DcsIds::DrawArgs::RightAfterburner, 0.75);
	expect_draw_arg(context, draw_args, DcsIds::DrawArgs::LeftNozzle, 0.2);
	expect_draw_arg(context, draw_args, DcsIds::DrawArgs::RightNozzle, 0.9);
}

void test_wheel_spin_draw_args(Tests::Context& context)
{
	DcsBridge::DrawArgState state = {};
	state.wheel_spin[0] = 0.1;
	state.wheel_spin[1] = 0.2;
	state.wheel_spin[2] = 0.3;
	const DrawArgumentBuffer draw_args = apply_draw_args(state);
	expect_draw_arg(
		context, draw_args, DcsIds::DrawArgs::NoseWheelSpin, 0.1);
	expect_draw_arg(
		context, draw_args, DcsIds::DrawArgs::LeftWheelSpin, 0.2);
	expect_draw_arg(
		context, draw_args, DcsIds::DrawArgs::RightWheelSpin, 0.3);
}

void test_draw_arg_clamping(Tests::Context& context)
{
	DcsBridge::DrawArgState state = {};
	state.gear_pos = 2.0;
	state.nose_wheel_steering = -2.0;
	state.elevator_command = 2.0;
	state.flaps_pos = 2.0;
	state.aileron_command = 2.0;
	state.rudder_command = 2.0;
	state.airbrake_pos = 2.0;
	state.left_afterburner_ratio = -2.0;
	state.right_afterburner_ratio = 2.0;
	state.left_nozzle_aperture = 2.0;
	state.right_nozzle_aperture = -2.0;
	state.slats_pos = 2.0;
	const DrawArgumentBuffer draw_args = apply_draw_args(state);
	expect_draw_arg(context, draw_args, DcsIds::DrawArgs::NoseGear, 1.0);
	expect_draw_arg(
		context, draw_args, DcsIds::DrawArgs::NoseWheelSteering, -1.0);
	expect_draw_arg(
		context, draw_args, DcsIds::DrawArgs::LeftElevator, 1.0);
	expect_draw_arg(
		context, draw_args, DcsIds::DrawArgs::RightFlaperon, 1.0);
	expect_draw_arg(
		context, draw_args, DcsIds::DrawArgs::LeftFlaperon, -1.0);
	expect_draw_arg(
		context, draw_args, DcsIds::DrawArgs::RudderPrimary, 1.0);
	expect_draw_arg(
		context, draw_args, DcsIds::DrawArgs::AirbrakePrimary, 1.0);
	expect_draw_arg(
		context, draw_args, DcsIds::DrawArgs::LeftAfterburner, 0.0);
	expect_draw_arg(
		context, draw_args, DcsIds::DrawArgs::RightAfterburner, 1.0);
	expect_draw_arg(context, draw_args, DcsIds::DrawArgs::LeftNozzle, 1.0);
	expect_draw_arg(context, draw_args, DcsIds::DrawArgs::RightNozzle, 0.0);
	expect_draw_arg(context, draw_args, DcsIds::DrawArgs::LeftSlat, 1.0);
}

void test_param_projection(Tests::Context& context)
{
	const DcsBridge::ParamExportState state =
		DcsBridge::make_param_export_state(make_frame_output());
	TEST_EXPECT(context, state.suspension_feedback_available);
	TEST_EXPECT(context, state.atmosphere_available);
	TEST_EXPECT(context, state.any_weight_on_wheels);
	TEST_EXPECT_NEAR(context, state.gear_pos, 0.8, kTolerance);
	TEST_EXPECT_NEAR(context, state.nose_wheel_steering, -0.25, kTolerance);
	TEST_EXPECT_NEAR(context, state.wheel_spin[2], 3.0, kTolerance);
	TEST_EXPECT_NEAR(context, state.wheel_brake_left, 0.35, kTolerance);
	TEST_EXPECT_NEAR(context, state.wheel_brake_right, 0.45, kTolerance);
	TEST_EXPECT_NEAR(context, state.pitch_input, 0.11, kTolerance);
	TEST_EXPECT_NEAR(context, state.roll_input, 0.12, kTolerance);
	TEST_EXPECT_NEAR(context, state.yaw_input, 0.13, kTolerance);
	TEST_EXPECT(context, state.left_engine_switch);
	TEST_EXPECT(context, !state.right_engine_switch);
	TEST_EXPECT_NEAR(context, state.left_throttle_input, 0.7, kTolerance);
	TEST_EXPECT_NEAR(context, state.right_throttle_input, 0.8, kTolerance);
	TEST_EXPECT_NEAR(context, state.left_throttle_output, 0.71, kTolerance);
	TEST_EXPECT_NEAR(context, state.right_throttle_output, 0.81, kTolerance);
	TEST_EXPECT_NEAR(context, state.left_engine_power_readout, 0.72, kTolerance);
	TEST_EXPECT_NEAR(context, state.right_engine_power_readout, 0.82, kTolerance);
	TEST_EXPECT_NEAR(context, state.left_thrust_force, 12000.0, kTolerance);
	TEST_EXPECT_NEAR(context, state.right_thrust_force, 13000.0, kTolerance);
	TEST_EXPECT_NEAR(context, state.atmosphere_temperature, 288.0, kTolerance);
	TEST_EXPECT_NEAR(context, state.internal_fuel, 900.0, kTolerance);
	TEST_EXPECT_NEAR(context, state.total_fuel, 1100.0, kTolerance);
	TEST_EXPECT_NEAR(context, state.total_fuel_flow, 1.5, kTolerance);
}

void test_unavailable_projection_metadata(Tests::Context& context)
{
	const DcsBridge::ParamExportState state =
		DcsBridge::make_param_export_state(Core::FrameOutput());
	TEST_EXPECT(context, !state.suspension_feedback_available);
	TEST_EXPECT(context, !state.atmosphere_available);
}
}

void run_output_adapter_tests(Tests::Context& context)
{
	test_draw_arg_projection(context);
	test_landing_gear_draw_args(context);
	test_rudder_draw_args(context);
	test_primary_control_surface_draw_args(context);
	test_opposed_flaperon_directions(context);
	test_secondary_surface_draw_args(context);
	test_engine_draw_args(context);
	test_wheel_spin_draw_args(context);
	test_draw_arg_clamping(context);
	test_param_projection(context);
	test_unavailable_projection_metadata(context);
}
