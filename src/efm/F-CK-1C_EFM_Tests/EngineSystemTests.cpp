#include "TestHarness.h"

#include "Core/Systems/Engine/EngineModel.h"
#include "Core/Systems/Engine/EngineConfig.h"

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

void test_digital_control_commands_are_separate_from_plant(
	Tests::Context& context)
{
	const auto& config = Core::Systems::fck1c_engine_config();
	const auto dry = Systems::command_dry_engine(0.5, 0.0, config);
	TEST_EXPECT(context, dry.throttle_output_target > 0.0);
	TEST_EXPECT_NEAR(
		context, dry.spool_time_constant_s, config.spool_up_tau, kTolerance);
	const auto afterburner = Systems::command_afterburner(
		{ 1.0, 1.0, 0.0, true }, false, config.afterburner);
	TEST_EXPECT(context, afterburner.lit);
	TEST_EXPECT(context, afterburner.ratio_target > 0.0);
	const auto fuel = Systems::command_fuel_flow(
		{ 0.5, 0.5, 0.0, 0.0 }, config);
	TEST_EXPECT_NEAR(
		context, fuel.flow_rate_kg_s,
		config.fuel_consumption_rate * (2.0 / 3.0), kTolerance);
}

}

void run_engine_system_tests(Tests::Context& context)
{
	test_engine_switches_and_throttle(context);
	test_engine_first_order(context);
	test_afterburner_ignition(context);
	test_digital_control_commands_are_separate_from_plant(context);
}
