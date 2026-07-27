#include "TestHarness.h"

#include "Core/Systems/FlightControlComputer/InputModel.h"

namespace
{
constexpr double kTolerance = 1e-9;

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
		Systems::compose_engine_throttle_cmd({ 0.2, 0.8, 0.0, true }),
		0.8, kTolerance);
	TEST_EXPECT_NEAR(context,
		Systems::compose_engine_throttle_cmd({ 0.2, 0.8, 0.5, false }),
		0.5, kTolerance);
}
}

void run_input_system_tests(Tests::Context& context)
{
	test_primary_axis_modes(context);
	test_axis_normalization(context);
	test_throttle_arbitration(context);
	test_fbw_throttle_composition(context);
}
