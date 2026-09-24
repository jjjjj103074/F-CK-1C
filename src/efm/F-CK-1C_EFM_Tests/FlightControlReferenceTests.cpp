#include "TestHarness.h"
#include "FlightControlCommandBindingTestHelper.h"

#include "Common/Units.h"
#include "Core/Systems/FlightControlComputer/CommandSystem/FlightControlCommandSystem.h"
#include "Core/Systems/FlightControlComputer/Configuration/FlightControlComputerConfig.h"

#include <cmath>
#include <variant>

namespace
{
constexpr double kTolerance = 1.0e-9;
constexpr double kTestAirspeedMps = 150.0;
constexpr double kTestAltitudeFt = 10000.0;
constexpr double kTestHeadingDeg = 90.0;

struct CommandSystemFixture
{
	const Core::Systems::FlightControlComputerConfig& config =
		Core::Systems::fck1c_flight_control_computer_config();
	::Systems::ModeAndGainScheduling scheduling{ config.values.mode_and_gain };
	Core::Systems::FlightControlCommandSystem system{{
		config.values.flight_control_laws.longitudinal,
		config.values.guidance_coordination,
		config.values.automatic_flight_control,
		{config.values.mode_and_gain.guidance_bank_limit_rad,
		 config.values.mode_and_gain.guidance_roll_rate_limit_rad_s},
		false }};
	Core::Systems::FlightControlCommandSystemInput input;

	CommandSystemFixture()
	{
		input.flight.dt_s = 1.0 / 64.0;
		input.flight.dynamic_pressure_pa = 5000.0;
		input.flight.indicated_airspeed_mps = kTestAirspeedMps;
		input.flight.flight_path_angle_available = true;
		input.flight.normal_acceleration_g = 1.0;
		input.flight.pressure_altitude_ft = kTestAltitudeFt;
		input.flight.pressure_altitude_available = true;
		input.flight.magnetic_heading_deg = kTestHeadingDeg;
		input.flight.magnetic_heading_available = true;
		input.flight.mach = 0.5;
		input.configuration = scheduling.update(
			{ input.flight.dt_s, input.flight.dynamic_pressure_pa,
				input.flight.mach, true, false });
	}

	void send(Core::CommandId id)
	{
		Tests::Fck1c::deliver_flight_control_command(
			system.command_bindings(), { id, 1.0 });
	}

	const Core::Systems::FlightControlCommandSystemResult& step()
	{
		return system.update(input);
	}
};

void engage_modes(
	CommandSystemFixture& fixture,
	Core::CommandId vertical,
	Core::CommandId lateral)
{
	(void)fixture.step();
	fixture.send(vertical);
	fixture.send(lateral);
	fixture.send(Core::CommandId::EngageAutopilot);
	(void)fixture.step();
}

void test_pilot_mapping_uses_physical_references(Tests::Context& context)
{
	CommandSystemFixture fixture;
	fixture.input.signals.pilot_pitch_normalized = 0.5;
	fixture.input.signals.pilot_roll_normalized = -0.4;
	fixture.input.signals.pilot_yaw_normalized = 0.3;
	const auto& reference = fixture.step().coordinated.reference;
	const auto* normal = std::get_if<Core::Systems::NormalAccelerationCommand>(
		&reference.longitudinal.command);
	TEST_EXPECT(context, normal != nullptr);
	TEST_EXPECT(context, normal != nullptr && normal->target_g > 1.0);
	TEST_EXPECT(
		context, reference.lateral_directional.roll_rate_reference_rad_s < 0.0);
	TEST_EXPECT(
		context, reference.lateral_directional.yaw_rate_feedforward_rad_s > 0.0);
}

void test_gear_handle_selects_single_pitch_rate_command(
	Tests::Context& context)
{
	CommandSystemFixture fixture;
	fixture.input.signals.landing_gear_handle_down = true;
	fixture.input.signals.pilot_pitch_normalized = 0.5;
	const auto& command = fixture.step().coordinated.reference
		.longitudinal.command;
	const auto* pitch_rate =
		std::get_if<Core::Systems::PitchRateCommand>(&command);
	TEST_EXPECT(context, pitch_rate != nullptr);
	TEST_EXPECT(
		context, pitch_rate != nullptr && pitch_rate->target_rad_s > 0.0);
}

void test_public_boundary_selects_axis_authority(Tests::Context& context)
{
	CommandSystemFixture fixture;
	engage_modes(fixture,
		Core::CommandId::SelectAutopilotAltitudeHold,
		Core::CommandId::SelectAutopilotHeadingSelect);
	const auto& selected = fixture.step().selected;
	TEST_EXPECT(context, selected.longitudinal_authority ==
		Core::Systems::AuthorityState::Automatic);
	TEST_EXPECT(context, selected.lateral_authority ==
		Core::Systems::AuthorityState::Automatic);
	TEST_EXPECT(context, std::holds_alternative<
		Core::Systems::AutomaticLongitudinalFlightReference>(
			selected.longitudinal));
	TEST_EXPECT(context, std::holds_alternative<
		Core::Systems::AutomaticLateralFlightReference>(selected.lateral));
}

void test_level_turn_compensation_is_applied_once(Tests::Context& context)
{
	CommandSystemFixture fixture;
	fixture.input.flight.roll_attitude_rad = Common::rad(30.0);
	engage_modes(fixture,
		Core::CommandId::SelectAutopilotAltitudeHold,
		Core::CommandId::SelectAutopilotRollAttitudeHold);
	const auto& result = fixture.step();
	const double expected_nz_g = 1.0 / std::cos(Common::rad(30.0));
	const auto& command = result.coordinated.reference.longitudinal.command;
	TEST_EXPECT_NEAR(context,
		std::get<Core::Systems::NormalAccelerationCommand>(command).target_g,
		expected_nz_g, kTolerance);
	TEST_EXPECT(context, result.coordinated.constraint.reason ==
		Core::Systems::ConstraintReason::None);
}

void test_combined_reference_reports_vertical_constraint(
	Tests::Context& context)
{
	CommandSystemFixture fixture;
	auto coordination = fixture.config.values.guidance_coordination;
	coordination.vertical_speed_error_to_acceleration_gain_s_inv = 1.0;
	Core::Systems::FlightControlCommandSystem constrained({
		fixture.config.values.flight_control_laws.longitudinal,
		coordination,
		fixture.config.values.automatic_flight_control,
		{fixture.config.values.mode_and_gain.guidance_bank_limit_rad,
		 fixture.config.values.mode_and_gain.guidance_roll_rate_limit_rad_s},
		false });
	(void)constrained.update(fixture.input);
	const auto bindings = constrained.command_bindings();
	Tests::Fck1c::deliver_flight_control_command(
		bindings, { Core::CommandId::SelectAutopilotAltitudeHold, 1.0 });
	Tests::Fck1c::deliver_flight_control_command(
		bindings, { Core::CommandId::SelectAutopilotRollAttitudeHold, 1.0 });
	Tests::Fck1c::deliver_flight_control_command(
		bindings, { Core::CommandId::EngageAutopilot, 1.0 });
	(void)constrained.update(fixture.input);
	fixture.input.flight.pressure_altitude_ft -= 1000.0;
	fixture.input.flight.vertical_speed_ft_s = -100.0;
	const auto& result = constrained.update(fixture.input);
	TEST_EXPECT(context, result.coordinated.constraint.reason ==
		Core::Systems::ConstraintReason::VerticalReferenceUnmaintainable);
	TEST_EXPECT(context, result.coordinated.constraint.vertical_constrained);
}

void test_right_bank_requests_right_yaw(Tests::Context& context)
{
	CommandSystemFixture fixture;
	fixture.input.flight.roll_attitude_rad = Common::rad(30.0);
	engage_modes(fixture,
		Core::CommandId::SelectAutopilotAltitudeHold,
		Core::CommandId::SelectAutopilotRollAttitudeHold);
	const auto& reference = fixture.step().coordinated.reference;
	// Core local-body +y is nose-left, so right coordinated yaw is negative.
	TEST_EXPECT(context,
		reference.lateral_directional.yaw_rate_feedforward_rad_s < 0.0);
}

struct VerticalCommandExpectation
{
	Core::CommandId vertical_mode =
		Core::CommandId::SelectAutopilotPitchAttitudeHold;
	bool gear_handle_down = false;
	bool expect_pitch_rate = false;
};

void expect_vertical_command_mode(
	Tests::Context& context,
	const VerticalCommandExpectation& expected)
{
	CommandSystemFixture fixture;
	fixture.input.signals.landing_gear_handle_down = expected.gear_handle_down;
	engage_modes(fixture, expected.vertical_mode,
		Core::CommandId::SelectAutopilotRollAttitudeHold);
	const auto& command =
		fixture.step().coordinated.reference.longitudinal.command;
	TEST_EXPECT(context,
		std::holds_alternative<Core::Systems::PitchRateCommand>(command) ==
			expected.expect_pitch_rate);
}

void test_gear_mode_selects_ap_longitudinal_command(Tests::Context& context)
{
	const Core::CommandId pitch_attitude =
		Core::CommandId::SelectAutopilotPitchAttitudeHold;
	const Core::CommandId altitude =
		Core::CommandId::SelectAutopilotAltitudeHold;
	expect_vertical_command_mode(context, { pitch_attitude, false, false });
	expect_vertical_command_mode(context, { altitude, false, false });
	expect_vertical_command_mode(context, { pitch_attitude, true, true });
	expect_vertical_command_mode(context, { altitude, true, true });
}
}

void run_flight_control_reference_tests(Tests::Context& context)
{
	test_pilot_mapping_uses_physical_references(context);
	test_gear_handle_selects_single_pitch_rate_command(context);
	test_public_boundary_selects_axis_authority(context);
	test_level_turn_compensation_is_applied_once(context);
	test_combined_reference_reports_vertical_constraint(context);
	test_right_bank_requests_right_yaw(context);
	test_gear_mode_selects_ap_longitudinal_command(context);
}
