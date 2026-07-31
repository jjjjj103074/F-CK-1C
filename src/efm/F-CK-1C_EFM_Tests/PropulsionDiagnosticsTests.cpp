#include "TestHarness.h"

#include "Core/Systems/PropulsionDiagnostics/PropulsionDiagnostics.h"

namespace
{
constexpr double kActiveCommand = 1.0;

void test_thrust_cut_commands_are_explicit(Tests::Context& context)
{
	Core::Systems::PropulsionDiagnostics diagnostics;
	diagnostics.handle_command({
		Core::CommandId::EnableThrustCutTest,
		kActiveCommand
	});
	TEST_EXPECT(context, diagnostics.intent().thrust_cut_requested);
	diagnostics.handle_command({
		Core::CommandId::DisableThrustCutTest,
		kActiveCommand
	});
	TEST_EXPECT(context, !diagnostics.intent().thrust_cut_requested);
	diagnostics.handle_command({
		Core::CommandId::ToggleThrustCutTest,
		kActiveCommand
	});
	TEST_EXPECT(context, diagnostics.intent().thrust_cut_requested);
}
}

void run_propulsion_diagnostics_tests(Tests::Context& context)
{
	test_thrust_cut_commands_are_explicit(context);
}
