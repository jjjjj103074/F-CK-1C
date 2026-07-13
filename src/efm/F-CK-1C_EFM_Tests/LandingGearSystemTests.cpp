#include "TestHarness.h"

#include "Systems/LandingGearSystem.h"

namespace
{
constexpr double kTolerance = 1e-9;

void test_brake_axis_normalization(Tests::Context& context)
{
	TEST_EXPECT_NEAR(context, Systems::normalize_brake_axis(-1.0), 0.0, kTolerance);
	TEST_EXPECT_NEAR(context, Systems::normalize_brake_axis(0.5), 0.5, kTolerance);
}

void test_gear_actuator(Tests::Context& context)
{
	Systems::LandingGearSystemState landing_gear;
	Systems::set_gear(landing_gear, true);
	Systems::update_gear_position(landing_gear);
	TEST_EXPECT_NEAR(context, landing_gear.position, 0.001, kTolerance);

	Systems::set_gear(landing_gear, false);
	Systems::update_gear_position(landing_gear);
	TEST_EXPECT_NEAR(context, landing_gear.position, 0.0, kTolerance);
}

void test_start_configuration(Tests::Context& context)
{
	Systems::LandingGearSystemState landing_gear;
	Systems::configure_ground_start_landing_gear(landing_gear);
	TEST_EXPECT(context, landing_gear.switch_down);
	TEST_EXPECT_NEAR(context, landing_gear.position, 1.0, kTolerance);
	TEST_EXPECT(context, landing_gear.wheels.nose_turn_enabled);

	Systems::configure_air_start_landing_gear(landing_gear);
	TEST_EXPECT(context, !landing_gear.switch_down);
	TEST_EXPECT_NEAR(context, landing_gear.position, 0.0, kTolerance);
	TEST_EXPECT(context, !landing_gear.wheels.nose_turn_enabled);
}
}

void run_landing_gear_system_tests(Tests::Context& context)
{
	test_brake_axis_normalization(context);
	test_gear_actuator(context);
	test_start_configuration(context);
}
