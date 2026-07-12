#include "TestHarness.h"

#include "Core/Fck1cEfm.h"

namespace
{
constexpr double kTolerance = 1e-9;

Core::Fck1cEfmConfig make_test_config()
{
	Core::Fck1cEfmConfig config;
	config.aerodynamics.wing_area = 24.26;
	config.left_engine_position = Common::Vec3(-3.793, -0.391, -0.716);
	config.right_engine_position = Common::Vec3(-3.793, -0.391, 0.716);
	return config;
}

void test_config_ownership(Tests::Context& context)
{
	Core::Fck1cEfm efm(make_test_config());
	TEST_EXPECT_NEAR(context, efm.config().aerodynamics.wing_area, 24.26, kTolerance);
	TEST_EXPECT_NEAR(context, efm.config().left_engine_position.z, -0.716, kTolerance);
	TEST_EXPECT_NEAR(context, efm.config().right_engine_position.z, 0.716, kTolerance);
}

void test_runtime_state_ownership(Tests::Context& context)
{
	Core::Fck1cEfm efm(make_test_config());
	efm.aircraft_state().current_mass = 9100.0;
	efm.systems().fuel.internal_fuel = 1200.0;
	efm.control_surfaces().elevator_command = 0.25;
	efm.gameplay().easy_flight = true;

	const Core::Fck1cEfm& read_only = efm;
	TEST_EXPECT_NEAR(context, read_only.aircraft_state().current_mass, 9100.0, kTolerance);
	TEST_EXPECT_NEAR(context, read_only.systems().fuel.internal_fuel, 1200.0, kTolerance);
	TEST_EXPECT_NEAR(context, read_only.control_surfaces().elevator_command, 0.25, kTolerance);
	TEST_EXPECT(context, read_only.gameplay().easy_flight);
}
}

void run_fck1c_efm_tests(Tests::Context& context)
{
	test_config_ownership(context);
	test_runtime_state_ownership(context);
}
