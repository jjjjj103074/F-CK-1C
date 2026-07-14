#include "TestHarness.h"

#include "Systems/EngineSystem.h"

namespace
{
constexpr double kTolerance = 1e-9;

void test_engine_switches_and_throttle(Tests::Context& context)
{
	Systems::EngineSystemState engines;
	Systems::set_both_engine_switches(engines, true);
	TEST_EXPECT(context, engines.left.switch_on);
	TEST_EXPECT(context, engines.right.switch_on);

	Systems::apply_engine_throttle_commands(engines, -1.0, 2.0);
	TEST_EXPECT_NEAR(context, engines.left.throttle_input, 0.0, kTolerance);
	TEST_EXPECT_NEAR(context, engines.right.throttle_input, 1.0, kTolerance);
}

void test_engine_first_order(Tests::Context& context)
{
	TEST_EXPECT_NEAR(
		context, Systems::engine_first_order(0.0, { 1.0, 1.0, 1.0 }),
		0.5, kTolerance);
	TEST_EXPECT_NEAR(
		context, Systems::engine_first_order(0.0, { 1.0, 0.0, 1.0 }),
		1.0, kTolerance);
}

void test_afterburner_ignition(Tests::Context& context)
{
	Systems::EngineSystemState engines;
	engines.left.switch_on = true;
	engines.left.throttle_input = 1.0;
	engines.left.throttle_output = 0.9;
	Systems::AfterburnerConfig afterburner;
	Systems::update_afterburner(engines.left, afterburner, 1.0);
	TEST_EXPECT(context, engines.left.afterburner_lit);
	TEST_EXPECT_NEAR(context, engines.left.afterburner_ratio, 1.0 / 3.0, kTolerance);
}

void test_engine_thrust_split(Tests::Context& context)
{
	constexpr double kTotalDryThrust = 54000.0;
	Systems::EngineSystemState engines;
	Systems::EngineSystemConfig config;
	engines.left.throttle_output = 1.0;
	engines.right.throttle_output = 1.0;
	Systems::update_engine_thrust_outputs(
		engines, config, { kTotalDryThrust, 1.0, 1.0, 1.0 });
	TEST_EXPECT_NEAR(context, engines.left.thrust_force, 27000.0, kTolerance);
	TEST_EXPECT_NEAR(context, engines.right.thrust_force, 27000.0, kTolerance);
}
}

void run_engine_system_tests(Tests::Context& context)
{
	test_engine_switches_and_throttle(context);
	test_engine_first_order(context);
	test_afterburner_ignition(context);
	test_engine_thrust_split(context);
}
