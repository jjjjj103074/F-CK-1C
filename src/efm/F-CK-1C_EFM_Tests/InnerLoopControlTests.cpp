#include "TestHarness.h"

#include "Core/Systems/FlightControlComputer/ControlLaws/InnerLoopControl.h"

namespace
{
constexpr double kTolerance = 1e-9;

void test_rate_reference_is_limited_per_axis(Tests::Context& context)
{
	const Systems::LimitedBodyRateReference result =
		Systems::limit_body_rate_reference(
			{ 2.0, -3.0, 0.5 },
			{ 1.0, 2.0, 1.0 });
	TEST_EXPECT_NEAR(context, result.value.roll_rate_rad_s, 1.0, kTolerance);
	TEST_EXPECT_NEAR(context, result.value.pitch_rate_rad_s, -2.0, kTolerance);
	TEST_EXPECT_NEAR(context, result.value.yaw_rate_rad_s, 0.5, kTolerance);
	TEST_EXPECT(context, result.constrained);
}

void test_inner_loop_axis_mapping(Tests::Context& context)
{
	Systems::InnerRateLoopStepInput input;
	input.dt_s = 0.01;
	input.reference = { 0.2, -0.3, 0.4 };
	input.gains = { 1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 2.0 };
	const Systems::InnerRateLoopResult result =
		Systems::update_inner_rate_loop({}, input);
	TEST_EXPECT_NEAR(
		context,
		result.surface_demand.aileron_command_normalized,
		0.2,
		kTolerance);
	TEST_EXPECT_NEAR(
		context,
		result.surface_demand.elevator_command_normalized,
		-0.3,
		kTolerance);
	TEST_EXPECT_NEAR(
		context,
		result.surface_demand.rudder_command_normalized,
		0.4,
		kTolerance);
}

void test_inner_loop_reports_saturation(Tests::Context& context)
{
	Systems::InnerRateLoopStepInput input;
	input.dt_s = 0.01;
	input.reference.roll_rate_rad_s = 2.0;
	input.gains = { 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 2.0 };
	const Systems::InnerRateLoopResult result =
		Systems::update_inner_rate_loop({}, input);
	TEST_EXPECT(context, result.anti_windup_active);
	TEST_EXPECT_NEAR(
		context,
		result.surface_demand.aileron_command_normalized,
		1.0,
		kTolerance);
}
}

void run_inner_loop_control_tests(Tests::Context& context)
{
	test_rate_reference_is_limited_per_axis(context);
	test_inner_loop_axis_mapping(context);
	test_inner_loop_reports_saturation(context);
}
