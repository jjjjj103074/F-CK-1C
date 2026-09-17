#include "TestHarness.h"

#include "Core/Systems/SecondaryFlightControls/SecondaryFlightControlModel.h"

namespace
{
constexpr double kTolerance = 1e-9;

void test_flap_modes(Tests::Context& context)
{
	Systems::AirframeDeviceState devices;
	Systems::toggle_flap_mode(devices);
	TEST_EXPECT(context, devices.flap_mode == Systems::FLAP_MODE_DOWN);
	Systems::toggle_flap_mode(devices);
	TEST_EXPECT(context, devices.flap_mode == Systems::FLAP_MODE_UP);
}

void test_auto_flap_schedule(Tests::Context& context)
{
	Systems::AirframeDeviceState devices;
	devices.flap_mode = Systems::FLAP_MODE_AUTO;
	Systems::AirframeDeviceUpdateInput input;
	input.true_airspeed_mps = Systems::kFlapsFullyRetractedSpeedMps;
	input.gear_position_normalized = 0.0;
	TEST_EXPECT_NEAR(context, Systems::compute_flap_target(devices, input), 0.0, kTolerance);

	input.gear_position_normalized = 1.0;
	TEST_EXPECT_NEAR(context, Systems::compute_flap_target(devices, input), 1.0, kTolerance);
}

void test_device_actuators(Tests::Context& context)
{
	Systems::AirframeDeviceState devices;
	devices.airbrake_switch = true;
	devices.flap_mode = Systems::FLAP_MODE_DOWN;
	const Systems::AirframeDeviceUpdateInput input = {};
	Systems::update_airframe_device_positions(devices, input);
	TEST_EXPECT_NEAR(
		context, devices.airbrake_position_normalized, 0.004, kTolerance);
	TEST_EXPECT_NEAR(
		context, devices.flaps_position_normalized, 0.002, kTolerance);
	TEST_EXPECT_NEAR(
		context, devices.slats_position_normalized, 0.003, kTolerance);
}
}

void run_airframe_device_system_tests(Tests::Context& context)
{
	test_flap_modes(context);
	test_auto_flap_schedule(context);
	test_device_actuators(context);
}
