#include "TestHarness.h"

#include "Core/Systems/FlightControlComputer/ControlLaws/ControlLaws.h"
#include "Core/Systems/FlightControlComputer/Configuration/FlightControlComputerConfig.h"

namespace
{
::Systems::FlightControlLawsInput nominal_input()
{
	const auto config =
		Core::Systems::fck1c_flight_control_computer_config();
	::Systems::ModeAndGainScheduling scheduling(config.values.mode_and_gain);
	::Systems::FlightControlLawsInput input;
	input.flight.dt_s = 1.0 / 64.0;
	input.flight.dynamic_pressure_pa = 5000.0;
	input.flight.mach = 0.5;
	input.flight.normal_acceleration_g = 1.0;
	input.maneuver.longitudinal.command =
		Core::Systems::NormalAccelerationCommand{ 1.0 };
	input.configuration = scheduling.update({
		input.flight.dt_s, input.flight.dynamic_pressure_pa,
		input.flight.mach, true, false });
	return input;
}

void test_public_laws_damp_measured_roll_rate(Tests::Context& context)
{
	const auto config =
		Core::Systems::fck1c_flight_control_computer_config();
	::Systems::FlightControlLaws laws(config.values.flight_control_laws);
	auto input = nominal_input();
	input.flight.roll_rate_rad_s = 0.3;
	const auto result = laws.update(input);
	TEST_EXPECT(context,
		result.normal_surface_demand.differential_flaperon_demand_rad < 0.0);
	TEST_EXPECT(context, !result.status.anti_windup_active);
}

void test_public_laws_report_inner_loop_saturation(Tests::Context& context)
{
	const auto config =
		Core::Systems::fck1c_flight_control_computer_config();
	::Systems::FlightControlLaws laws(config.values.flight_control_laws);
	auto input = nominal_input();
	input.flight.roll_rate_rad_s = 100.0;
	const auto result = laws.update(input);
	TEST_EXPECT(context, result.status.anti_windup_active);
	TEST_EXPECT(context,
		result.normal_surface_demand.differential_flaperon_demand_rad < 0.0);
}
}

void run_inner_loop_control_tests(Tests::Context& context)
{
	test_public_laws_damp_measured_roll_rate(context);
	test_public_laws_report_inner_loop_saturation(context);
}
