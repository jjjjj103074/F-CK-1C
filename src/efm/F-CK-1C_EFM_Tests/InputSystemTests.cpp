#include "TestHarness.h"

#include "Core/Systems/PilotControls/InputModel.h"
#include "Core/Systems/FlightControlComputer/Util/ThrottleCommandComposition.h"

namespace
{
constexpr double kTolerance = 1e-9;
constexpr double kReferenceAxisDt = 1.0 / 64.0;
constexpr double kHalfReferenceAxisDt = kReferenceAxisDt / 2.0;

void test_primary_axis_modes(Tests::Context& context)
{
	Systems::PrimaryControlState controls;
	Systems::set_pitch_axis_input(controls, 2.0);
	TEST_EXPECT_NEAR(context, controls.pitch.input, 1.0, kTolerance);
	TEST_EXPECT(context, controls.pitch.analog);
	TEST_EXPECT(context, controls.pitch.discrete == 0);

	Systems::set_pitch_discrete_input(controls, -1);
	TEST_EXPECT(context, !controls.pitch.analog);
	TEST_EXPECT(context, controls.pitch.discrete == -1);
}

void test_axis_normalization(Tests::Context& context)
{
	TEST_EXPECT_NEAR(context, Systems::normalize_throttle_axis(-1.0, false), 0.0, kTolerance);
	TEST_EXPECT_NEAR(context, Systems::normalize_throttle_axis(1.0, false), 1.0, kTolerance);
	TEST_EXPECT_NEAR(context, Systems::normalize_throttle_axis(-1.0, true), 1.0, kTolerance);
}

void test_virtual_axes_use_elapsed_time(Tests::Context& context)
{
	Systems::PrimaryControlState coarse;
	Systems::set_pitch_discrete_input(coarse, 1);
	Systems::set_roll_discrete_input(coarse, -1);
	Systems::set_yaw_discrete_input(coarse, 1);
	Systems::PrimaryControlState fine = coarse;
	coarse = Systems::update_primary_control_inputs(
		coarse, kReferenceAxisDt);
	fine = Systems::update_primary_control_inputs(
		fine, kHalfReferenceAxisDt);
	fine = Systems::update_primary_control_inputs(
		fine, kHalfReferenceAxisDt);
	TEST_EXPECT_NEAR(context, coarse.pitch.input, fine.pitch.input, kTolerance);
	TEST_EXPECT_NEAR(context, coarse.roll.input, fine.roll.input, kTolerance);
	TEST_EXPECT_NEAR(context, coarse.yaw.input, fine.yaw.input, kTolerance);
}

void test_throttle_arbitration(Tests::Context& context)
{
	Systems::ThrottleInputState throttles;
	Systems::reset_throttle_inputs(throttles, 0.25, 0.75);
	Systems::update_pilot_throttle_cmds(throttles);
	TEST_EXPECT_NEAR(context, throttles.left.pilot_cmd, 0.25, kTolerance);
	TEST_EXPECT_NEAR(context, throttles.right.pilot_cmd, 0.75, kTolerance);

	Systems::set_common_throttle_axis(throttles, 0.0);
	Systems::update_pilot_throttle_cmds(throttles);
	TEST_EXPECT_NEAR(context, throttles.left.pilot_cmd, 0.5, kTolerance);
	TEST_EXPECT_NEAR(context, throttles.right.pilot_cmd, 0.5, kTolerance);

	Systems::step_left_keyboard_throttle(throttles, 0.1);
	Systems::update_pilot_throttle_cmds(throttles);
	TEST_EXPECT_NEAR(context, throttles.left.pilot_cmd, 0.6, kTolerance);
}

void test_fbw_throttle_composition(Tests::Context& context)
{
	TEST_EXPECT_NEAR(context,
		Systems::compose_engine_throttle_command({ 0.2, 0.8, 0.0, true }),
		0.8, kTolerance);
	TEST_EXPECT_NEAR(context,
		Systems::compose_engine_throttle_command({ 0.2, 0.8, 0.5, false }),
		0.5, kTolerance);
}
}

void run_input_system_tests(Tests::Context& context)
{
	test_primary_axis_modes(context);
	test_axis_normalization(context);
	test_virtual_axes_use_elapsed_time(context);
	test_throttle_arbitration(context);
	test_fbw_throttle_composition(context);
}
