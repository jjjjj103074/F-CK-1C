#include "TestHarness.h"

#include "Core/Systems/FlightControlComputer/InputSignalManagement.h"

#include <limits>
#include <stdexcept>

namespace
{
constexpr double kTolerance = 1e-9;

Core::Systems::RawFlightControlInput valid_input()
{
	Core::Systems::RawFlightControlInput input;
	input.dt_s = 0.01;
	input.alpha_limit_deg = 20.0;
	input.observation.normal_acceleration_g = 1.0;
	input.observation.dynamic_pressure_pa = 5000.0;
	return input;
}

void test_axis_signs_and_stick_conditioning(Tests::Context& context)
{
	Systems::FBWControllerConfig config;
	config.signal_filter_tau = 0.0;
	config.qbar_filter_tau = 0.0;
	config.cat1.command_shape_tau = 0.0;
	config.cat1.command_shape_rate = 1000.0;
	config.cat1.stick_expo = 0.0;
	Core::Systems::InputSignalManagement manager(config);
	auto raw = valid_input();
	raw.observation.roll_rad = 0.2;
	raw.observation.pitch_rad = -0.1;
	raw.observation.vertical_speed_mps = 3.0;
	raw.pilot.roll_axis_normalized = 0.3;
	raw.pilot.pitch_axis_normalized = -0.4;
	raw.pilot.yaw_axis_normalized = 0.2;
	const auto result = manager.condition(raw, Systems::FBW_CAT1);
	TEST_EXPECT_NEAR(context, result.roll_attitude_rad, 0.2, kTolerance);
	TEST_EXPECT_NEAR(context, result.pitch_attitude_rad, -0.1, kTolerance);
	TEST_EXPECT_NEAR(context, result.vertical_speed_mps, 3.0, kTolerance);
	TEST_EXPECT_NEAR(context, result.pilot_roll_normalized, 0.3, kTolerance);
	TEST_EXPECT_NEAR(context, result.pilot_pitch_normalized, -0.4, kTolerance);
	TEST_EXPECT_NEAR(context, result.pilot_yaw_normalized, 0.2, kTolerance);
}

void test_filter_uses_scheduled_dt(Tests::Context& context)
{
	Systems::FBWControllerConfig config;
	config.signal_filter_tau = 1.0;
	Core::Systems::InputSignalManagement short_step(config);
	Core::Systems::InputSignalManagement long_step(config);
	auto raw = valid_input();
	raw.observation.roll_rad = 1.0;
	raw.dt_s = 0.01;
	const auto short_result = short_step.condition(raw, Systems::FBW_CAT1);
	raw.dt_s = 0.02;
	const auto long_result = long_step.condition(raw, Systems::FBW_CAT1);
	TEST_EXPECT_NEAR(
		context, short_result.roll_attitude_rad, 0.01 / 1.01, kTolerance);
	TEST_EXPECT_NEAR(
		context, long_result.roll_attitude_rad, 0.02 / 1.02, kTolerance);
}

void test_heading_is_wrapped(Tests::Context& context)
{
	Systems::FBWControllerConfig config;
	config.signal_filter_tau = 0.0;
	Core::Systems::InputSignalManagement manager(config);
	auto raw = valid_input();
	raw.observation.heading_rad = 3.5;
	const auto result = manager.condition(raw, Systems::FBW_CAT1);
	TEST_EXPECT(context, result.heading_rad < 0.0);
	TEST_EXPECT_NEAR(context, result.heading_rad, -2.7831853071795862, kTolerance);
}

void test_invalid_input_is_exposed(Tests::Context& context)
{
	Core::Systems::InputSignalManagement manager({});
	auto raw = valid_input();
	raw.observation.mach = std::numeric_limits<double>::quiet_NaN();
	bool rejected = false;
	try
	{
		manager.condition(raw, Systems::FBW_CAT1);
	}
	catch (const std::invalid_argument&)
	{
		rejected = true;
	}
	TEST_EXPECT(context, rejected);
}
}

void run_input_signal_management_tests(Tests::Context& context)
{
	test_axis_signs_and_stick_conditioning(context);
	test_filter_uses_scheduled_dt(context);
	test_heading_is_wrapped(context);
	test_invalid_input_is_exposed(context);
}
