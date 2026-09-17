#include "TestHarness.h"

#include "Core/Systems/FlightControlComputer/Input/InputSignalManagement.h"

#include <limits>
#include <stdexcept>

namespace
{
constexpr double kTolerance = 1e-9;
constexpr double kPilotShapingDtS = 1.0 / 32.0;

Core::Systems::RawFlightControlInput valid_input()
{
	Core::Systems::RawFlightControlInput input;
	input.dt_s = 0.01;
	input.observation.normal_acceleration_g = 1.0;
	input.observation.dynamic_pressure_pa = 5000.0;
	return input;
}

Systems::PilotInputShapingConfig immediate_shaping()
{
	Systems::PilotInputShapingConfig result;
	result.command_rate_normalized_s = 1000.0;
	return result;
}

Core::Systems::InputSignalManagementStepInput step_input(
	const Core::Systems::RawFlightControlInput& raw,
	bool update_pilot_shaping = true)
{
	return {
		raw,
		immediate_shaping(),
		update_pilot_shaping,
		kPilotShapingDtS
	};
}

void test_axis_signs_and_stick_conditioning(Tests::Context& context)
{
	Systems::InputSignalManagementConfig config;
	config.signal_filter_time_constant_s = 0.0;
	config.dynamic_pressure_filter_time_constant_s = 0.0;
	config.normal_acceleration_filter_time_constant_s = 0.0;
	Core::Systems::InputSignalManagement manager(config);
	auto raw = valid_input();
	raw.observation.roll_rad = 0.2;
	raw.observation.pitch_rad = -0.1;
	raw.observation.vertical_speed_ft_s = 3.0;
	raw.pilot.roll_axis_normalized = 0.3;
	raw.pilot.pitch_axis_normalized = -0.4;
	raw.pilot.yaw_axis_normalized = 0.2;
	const auto result = manager.update(step_input(raw));
	TEST_EXPECT_NEAR(context, result.observation.roll_rad, 0.2, kTolerance);
	TEST_EXPECT_NEAR(context, result.observation.pitch_rad, -0.1, kTolerance);
	TEST_EXPECT_NEAR(
		context, result.observation.vertical_speed_ft_s, 3.0, kTolerance);
	TEST_EXPECT_NEAR(context, result.pilot_roll_normalized, 0.3, kTolerance);
	TEST_EXPECT_NEAR(context, result.pilot_pitch_normalized, -0.4, kTolerance);
	TEST_EXPECT_NEAR(context, result.pilot_yaw_normalized, 0.2, kTolerance);
}

void test_filter_uses_scheduled_dt(Tests::Context& context)
{
	Systems::InputSignalManagementConfig config;
	config.signal_filter_time_constant_s = 1.0;
	Core::Systems::InputSignalManagement short_step(config);
	Core::Systems::InputSignalManagement long_step(config);
	auto raw = valid_input();
	(void)short_step.update(step_input(raw));
	(void)long_step.update(step_input(raw));
	raw.observation.roll_rad = 1.0;
	raw.dt_s = 0.01;
	const auto short_result = short_step.update(step_input(raw));
	raw.dt_s = 0.02;
	const auto long_result = long_step.update(step_input(raw));
	TEST_EXPECT_NEAR(
		context, short_result.observation.roll_rad, 0.01 / 1.01, kTolerance);
	TEST_EXPECT_NEAR(
		context, long_result.observation.roll_rad, 0.02 / 1.02, kTolerance);
}

void test_heading_is_wrapped_in_degrees(Tests::Context& context)
{
	Systems::InputSignalManagementConfig config;
	config.signal_filter_time_constant_s = 0.0;
	Core::Systems::InputSignalManagement manager(config);
	auto raw = valid_input();
	raw.observation.magnetic_heading_available = true;
	raw.observation.magnetic_heading_deg = 370.0;
	const auto result = manager.update(step_input(raw));
	TEST_EXPECT(context, result.observation.magnetic_heading_available);
	TEST_EXPECT_NEAR(
		context, result.observation.magnetic_heading_deg, 10.0, kTolerance);
}

void test_pilot_shaping_is_zero_order_held(Tests::Context& context)
{
	Systems::InputSignalManagementConfig config;
	config.signal_filter_time_constant_s = 0.0;
	Core::Systems::InputSignalManagement manager(config);
	auto raw = valid_input();
	raw.pilot.roll_axis_normalized = 0.25;
	const auto first = manager.update(step_input(raw));
	const double held = first.pilot_roll_normalized;
	raw.pilot.roll_axis_normalized = 0.75;
	const auto second = manager.update(step_input(raw, false));
	TEST_EXPECT_NEAR(context, second.pilot_roll_normalized, held, kTolerance);
	TEST_EXPECT_NEAR(
		context, second.pilot_roll_raw_normalized, 0.75, kTolerance);
}

void test_invalid_input_is_exposed(Tests::Context& context)
{
	Core::Systems::InputSignalManagement manager({});
	auto raw = valid_input();
	raw.observation.mach = std::numeric_limits<double>::quiet_NaN();
	bool rejected = false;
	try
	{
		(void)manager.update(step_input(raw));
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
	test_heading_is_wrapped_in_degrees(context);
	test_pilot_shaping_is_zero_order_held(context);
	test_invalid_input_is_exposed(context);
}
