#include "TestHarness.h"

#include "Core/Systems/FlightControlComputer/Diagnostics/FlightControlDiagnostics.h"
#include "Core/Systems/FlightControlComputer/Output/FlightControlOutputSystem.h"

#include <limits>
#include <stdexcept>

namespace
{
constexpr double kTolerance = 1.0e-9;
constexpr double kFccDtS = 1.0 / 64.0;

Core::Systems::FlightControlOutputInput output_input()
{
	Core::Systems::FlightControlOutputInput input;
	input.dt_s = kFccDtS;
	input.laws.normal_surface_demand = { 0.1, -0.05, 0.02 };
	input.laws.developer_direct_surface_demand = { -0.2, 0.1, -0.08 };
	input.actuator.symmetric_stabilator.position_rad = 0.1;
	input.actuator.differential_flaperon.position_rad = -0.05;
	input.actuator.rudder.position_rad = 0.02;
	return input;
}

void test_normal_selection_routes_physical_demands_and_feedback(
	Tests::Context& context)
{
	Core::Systems::FlightControlOutputSystem output({}, {});
	const auto result = output.update(output_input());
	TEST_EXPECT_NEAR(context,
		result.actuator_command.symmetric_stabilator_demand_rad,
		0.1, kTolerance);
	TEST_EXPECT(context, result.status.command_valid);
	TEST_EXPECT(context, result.status.actuator_feedback_valid);
	TEST_EXPECT(context, result.status.actuator_tracking_consistent);
}

void test_developer_direct_selection_uses_bumpless_transfer(
	Tests::Context& context)
{
	Core::Systems::FlightControlOutputSystem output({}, {});
	auto input = output_input();
	(void)output.update(input);
	input.selection =
		Core::Systems::ElectronicControlLawSelection::DeveloperDirect;
	const auto result = output.update(input);
	const double command =
		result.actuator_command.symmetric_stabilator_demand_rad;
	TEST_EXPECT(context, command < 0.1);
	TEST_EXPECT(context, command > -0.2);
	TEST_EXPECT(context, result.status.selection_transition_active);
}

void test_invalid_actuator_feedback_is_rejected(Tests::Context& context)
{
	Core::Systems::FlightControlOutputSystem output({}, {});
	auto input = output_input();
	input.actuator.rudder.position_rad =
		std::numeric_limits<double>::quiet_NaN();
	bool rejected = false;
	try
	{
		(void)output.update(input);
	}
	catch (const std::domain_error&)
	{
		rejected = true;
	}
	TEST_EXPECT(context, rejected);
}

void test_electronic_and_physical_saturation_are_distinct(
	Tests::Context& context)
{
	const ::Systems::SurfaceCommandMixerConfig limits;
	Core::Systems::FlightControlOutputSystem output(limits, {});
	auto input = output_input();
	input.laws.normal_surface_demand.symmetric_stabilator_demand_rad =
		2.0 * limits.symmetric_stabilator_limit_rad;
	const auto result = output.update(input);
	TEST_EXPECT_NEAR(context,
		result.actuator_command.symmetric_stabilator_demand_rad,
		limits.symmetric_stabilator_limit_rad, kTolerance);
	TEST_EXPECT(context, result.status.electronic_command_saturated);
	TEST_EXPECT(context, !result.status.actuator_saturated);
}

void test_physical_saturation_does_not_imply_electronic_limiting(
	Tests::Context& context)
{
	Core::Systems::FlightControlOutputSystem output({}, {});
	auto input = output_input();
	input.actuator.any_saturated = true;
	const auto result = output.update(input);
	TEST_EXPECT(context, !result.status.electronic_command_saturated);
	TEST_EXPECT(context, result.status.actuator_saturated);
	TEST_EXPECT(context, !result.status.protection_authority_exhausted);
}

void test_control_law_saturation_reaches_output_status(
	Tests::Context& context)
{
	Core::Systems::FlightControlOutputSystem output({}, {});
	auto input = output_input();
	input.laws.status.electronic_command_saturated = true;
	input.laws.status.angle_of_attack_limit_active = true;
	const auto result = output.update(input);
	TEST_EXPECT(context, result.status.electronic_command_saturated);
	TEST_EXPECT(context, result.status.protection_authority_exhausted);
}

void test_other_axis_saturation_does_not_exhaust_pitch_authority(
	Tests::Context& context)
{
	const ::Systems::SurfaceCommandMixerConfig limits;
	Core::Systems::FlightControlOutputSystem output(limits, {});
	auto input = output_input();
	input.laws.status.angle_of_attack_limit_active = true;
	input.actuator.differential_flaperon.position_rad =
		limits.differential_flaperon_limit_rad;
	input.actuator.differential_flaperon.position_limit =
		Core::FlightControlPositionLimit::Positive;
	input.actuator.differential_flaperon.saturated = true;
	input.actuator.any_saturated = true;
	const auto result = output.update(input);
	TEST_EXPECT(context, result.status.actuator_saturated);
	TEST_EXPECT(context, !result.status.protection_authority_exhausted);
}

void test_actuator_position_limit_owns_pitch_authority_status(
	Tests::Context& context)
{
	Core::Systems::FlightControlOutputSystem output({}, {});
	auto input = output_input();
	input.laws.status.angle_of_attack_limit_active = true;
	input.actuator.symmetric_stabilator.position_rad = 0.05;
	input.actuator.symmetric_stabilator.position_limit =
		Core::FlightControlPositionLimit::Positive;
	input.actuator.symmetric_stabilator.at_position_limit = true;
	input.actuator.symmetric_stabilator.saturated = true;
	input.actuator.any_saturated = true;
	const auto result = output.update(input);
	TEST_EXPECT(context, result.status.protection_authority_exhausted);
}

void test_physical_saturation_reaches_diagnostics_snapshot(
	Tests::Context& context)
{
	Core::Systems::FlightControlCommandSystemResult command;
	::Systems::FlightControlLawsResult laws;
	::Systems::ActiveFlightControlConfiguration configuration;
	::Systems::ManagedFlightControlSignals signals;
	Core::Systems::FlightControlOutputStatus output;
	Core::FlightControlActuatorCommand actuator;
	output.actuator_saturated = true;
	signals.observation.normal_acceleration_g = 2.25;
	Core::Systems::FlightControlDiagnostics diagnostics(false, false);
	const auto& snapshot = diagnostics.update({
		0, 0, 0, 0, 0, false, false, command, laws, configuration,
		signals, output, actuator });
	TEST_EXPECT(context, snapshot.actuator_saturated);
	TEST_EXPECT_NEAR(
		context, snapshot.filtered_normal_acceleration_g, 2.25, kTolerance);
	TEST_EXPECT(context, !snapshot.control_authority_limited);
	TEST_EXPECT(context, !snapshot.electronic_command_saturated);
}

void test_non_recovering_alpha_qualifies_control_authority(
	Tests::Context& context)
{
	::Systems::FlightControlDiagnosticsConfig config;
	config.control_authority_persistence_s = 0.02;
	config.minimum_alpha_recovery_rate_rad_s = 0.0;
	Core::Systems::FlightControlCommandSystemResult command;
	::Systems::FlightControlLawsResult laws;
	::Systems::ActiveFlightControlConfiguration configuration;
	::Systems::ManagedFlightControlSignals signals;
	Core::Systems::FlightControlOutputStatus output;
	Core::FlightControlActuatorCommand actuator;
	signals.dt_s = 0.01;
	laws.status.angle_of_attack_limit_active = true;
	laws.diagnostics.limited_pitch_effort = -1.0;
	output.protection_authority_exhausted = true;
	Core::Systems::FlightControlDiagnostics diagnostics(false, false, config);
	Core::Systems::FlightControlDiagnosticsInput input = {
		0, 0, 0, 0, 0, false, false, command, laws, configuration,
		signals, output, actuator };
	(void)diagnostics.update(input);
	signals.observation.angle_of_attack_rad += 0.01;
	(void)diagnostics.update(input);
	signals.observation.angle_of_attack_rad += 0.01;
	TEST_EXPECT(context, diagnostics.update(input).control_authority_limited);
}
}

void run_flight_control_output_system_tests(Tests::Context& context)
{
	test_normal_selection_routes_physical_demands_and_feedback(context);
	test_developer_direct_selection_uses_bumpless_transfer(context);
	test_invalid_actuator_feedback_is_rejected(context);
	test_electronic_and_physical_saturation_are_distinct(context);
	test_physical_saturation_does_not_imply_electronic_limiting(context);
	test_control_law_saturation_reaches_output_status(context);
	test_other_axis_saturation_does_not_exhaust_pitch_authority(context);
	test_actuator_position_limit_owns_pitch_authority_status(context);
	test_physical_saturation_reaches_diagnostics_snapshot(context);
	test_non_recovering_alpha_qualifies_control_authority(context);
}
