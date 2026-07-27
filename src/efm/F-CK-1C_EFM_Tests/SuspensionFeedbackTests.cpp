#include "TestHarness.h"

#include "Core/Systems/LandingGear/SuspensionFeedback.h"

namespace
{
constexpr double kTolerance = 1e-6;

void test_feedback_value_input(Tests::Context& context)
{
	Core::Systems::SuspensionFeedbackState state;
	const bool updated = Core::Systems::update_suspension_feedback(
		state, { 1, 0.2, Common::Vec3(3.0, 4.0, 0.0) });
	TEST_EXPECT(context, updated);
	TEST_EXPECT(context, state.feedback_valid[1]);
	TEST_EXPECT(context, state.weight_on_wheel[1]);
	TEST_EXPECT_NEAR(
		context, state.force_magnitude[1], 5.0, kTolerance);
	TEST_EXPECT(
		context,
		!Core::Systems::update_suspension_feedback(
			state, { 3, 0.0, {} }));
}
}

void run_suspension_feedback_tests(Tests::Context& context)
{
	test_feedback_value_input(context);
}
