#include "TestHarness.h"

#include "DcsBridge/DcsSnapshots.h"

namespace
{
constexpr double kTolerance = 1e-9;

Core::FrameOutput make_frame_output()
{
	Core::FrameOutput output;
	output.availability.suspension = { true, false, false };
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
	output.fuel = { 900.0, 200.0, 1100.0 };
	return output;
}

void test_draw_arg_snapshot(Tests::Context& context)
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

void test_param_snapshot(Tests::Context& context)
{
	const DcsBridge::ParamExportState state =
		DcsBridge::make_param_export_state(make_frame_output());
	TEST_EXPECT(context, state.suspension_feedback_available);
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
}

void test_param_snapshot_without_suspension(Tests::Context& context)
{
	const DcsBridge::ParamExportState state =
		DcsBridge::make_param_export_state(Core::FrameOutput());
	TEST_EXPECT(context, !state.suspension_feedback_available);
}

}

void run_dcs_snapshots_tests(Tests::Context& context)
{
	test_draw_arg_snapshot(context);
	test_param_snapshot(context);
	test_param_snapshot_without_suspension(context);
}
