#include "TestHarness.h"

#include "Systems/SuspensionSystem.h"

namespace
{
constexpr double kTolerance = 1e-6;

void test_feedback_value_input(Tests::Context& context)
{
	Systems::SuspensionSystemState state;
	const bool updated = Systems::update_suspension_feedback(
		state, { 1, 0.2, Common::Vec3(3.0, 4.0, 0.0) });
	TEST_EXPECT(context, updated);
	TEST_EXPECT(context, state.feedback_valid[1]);
	TEST_EXPECT(context, state.wow[1]);
	TEST_EXPECT_NEAR(context, state.force_mag[1], 5.0, kTolerance);
	TEST_EXPECT(context, !Systems::update_suspension_feedback(state, { 3, 0.0, {} }));
}

void test_fallback_force_split(Tests::Context& context)
{
	Systems::SuspensionSystemConfig config;
	config.enable_fallback_ground_forces = true;
	Systems::SuspensionFallbackInput input;
	input.altitude_agl = 2.25;
	input.gear_pos = 1.0;
	input.current_mass = 10000.0;
	input.velocity_body_x = 10.0;
	Systems::SuspensionSystemState state;
	int force_count = 0;
	const double total = Systems::apply_fallback_ground_forces(
		state,
		Systems::make_suspension_fallback_context(config, input),
		[&force_count](const Common::Vec3&, const Common::Vec3&) { ++force_count; });
	TEST_EXPECT_NEAR(context, total, 145920.0, kTolerance);
	TEST_EXPECT(context, state.fallback_wow[1]);
	TEST_EXPECT(context, state.fallback_wow[2]);
	TEST_EXPECT(context, force_count == 3);
}
}

void run_suspension_system_tests(Tests::Context& context)
{
	test_feedback_value_input(context);
	test_fallback_force_split(context);
}
