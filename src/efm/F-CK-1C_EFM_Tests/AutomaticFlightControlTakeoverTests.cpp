#include "TestHarness.h"
#include "AutomaticFlightControlTestSupport.h"

#include "Common/Units.h"

namespace
{
using namespace AutomaticFlightControlTestSupport;

constexpr double kPaddlePitchChangeDeg = 3.0;
constexpr double kPaddleAltitudeChangeFt = 100.0;
constexpr double kPaddleHeadingChangeDeg = 5.0;
constexpr double kPaddleRollChangeDeg = 4.0;
constexpr double kTestStickInputNormalized = 0.5;
constexpr double kReleasedCommandValue = 0.0;

struct VerticalPaddleCase
{
	CommandId select_command;
	AutomaticFlightControlVerticalMode expected_mode;
};

void expect_vertical_target(
	Tests::Context& context,
	const Core::AutomaticFlightControlSnapshot& snapshot,
	const AutomaticFlightControlObservation& observation)
{
	switch (snapshot.vertical_mode)
	{
	case AutomaticFlightControlVerticalMode::PitchAttitudeHold:
		TEST_EXPECT_NEAR(
			context, snapshot.target_pitch_rad,
			observation.pitch_rad, kTolerance);
		break;
	case AutomaticFlightControlVerticalMode::AltitudeHold:
		TEST_EXPECT_NEAR(
			context, snapshot.target_altitude_ft,
			observation.pressure_altitude_ft, kTolerance);
		break;
	default:
		TEST_EXPECT(context, false);
		break;
	}
}

void test_paddle_release_recaptures_each_vertical_mode(
	Tests::Context& context)
{
	const VerticalPaddleCase cases[] = {
		{ CommandId::SelectAutopilotPitchAttitudeHold,
			AutomaticFlightControlVerticalMode::PitchAttitudeHold },
		{ CommandId::SelectAutopilotAltitudeHold,
			AutomaticFlightControlVerticalMode::AltitudeHold }
	};
	for (const VerticalPaddleCase& test_case : cases)
	{
		AutomaticFlightControl control = make_control();
		auto observation = nominal_observation();
		prime_and_engage(control, observation);
		send(control, test_case.select_command);
		(void)step(control, observation);
		send(control, CommandId::SetAutopilotBypass);
		(void)step(control, observation);
		observation.pitch_rad += Common::rad(kPaddlePitchChangeDeg);
		observation.pressure_altitude_ft += kPaddleAltitudeChangeFt;
		send(
			control, CommandId::SetAutopilotBypass,
			kReleasedCommandValue);
		(void)step(control, observation);
		TEST_EXPECT(
			context, control.snapshot().vertical_mode == test_case.expected_mode);
		expect_vertical_target(context, control.snapshot(), observation);
	}
}

void test_paddle_release_applies_each_lateral_mode_rule(
	Tests::Context& context)
{
	AutomaticFlightControl roll_attitude = make_control();
	auto observation = nominal_observation();
	prime_and_engage(roll_attitude, observation);
	send(roll_attitude, CommandId::SetAutopilotBypass);
	(void)step(roll_attitude, observation);
	observation.roll_rad += Common::rad(kPaddleRollChangeDeg);
	send(
		roll_attitude, CommandId::SetAutopilotBypass,
		kReleasedCommandValue);
	const auto roll_reference = step(roll_attitude, observation);
	TEST_EXPECT(
		context,
		roll_reference.lateral_authority ==
			Core::Systems::AuthorityState::Automatic);
	TEST_EXPECT(
		context,
		roll_reference.bank_angle_reference_rad < 0.0);

	AutomaticFlightControl heading_select = make_control();
	observation = nominal_observation();
	(void)step(heading_select, observation);
	send(heading_select, CommandId::SelectAutopilotHeadingSelect);
	prime_and_engage(heading_select, observation);
	send(heading_select, CommandId::IncreaseAutopilotHeadingSelect);
	(void)step(heading_select, observation);
	const int selected_heading = heading_select.snapshot().target_heading_deg;
	send(heading_select, CommandId::SetAutopilotBypass);
	(void)step(heading_select, observation);
	observation.magnetic_heading_deg += kPaddleHeadingChangeDeg;
	send(
		heading_select, CommandId::SetAutopilotBypass,
		kReleasedCommandValue);
	(void)step(heading_select, observation);
	TEST_EXPECT(
		context,
		heading_select.snapshot().target_heading_deg == selected_heading);
}

void test_path_modes_ignore_attitude_stick_steering(
	Tests::Context& context)
{
	AutomaticFlightControl altitude = make_control();
	auto observation = nominal_observation();
	prime_and_engage(altitude, observation);
	send(altitude, CommandId::SelectAutopilotAltitudeHold);
	observation.conditioned_pitch_input_normalized =
		kTestStickInputNormalized;
	const auto altitude_reference = step(altitude, observation);
	TEST_EXPECT(
		context, altitude_reference.longitudinal_authority ==
			Core::Systems::AuthorityState::Automatic);

	AutomaticFlightControl heading = make_control();
	observation = nominal_observation();
	(void)step(heading, observation);
	send(heading, CommandId::SelectAutopilotHeadingSelect);
	prime_and_engage(heading, observation);
	observation.conditioned_roll_input_normalized =
		kTestStickInputNormalized;
	const auto heading_reference = step(heading, observation);
	TEST_EXPECT(
		context, heading_reference.lateral_authority ==
			Core::Systems::AuthorityState::Automatic);
}

void test_roll_attitude_hold_uses_stick_steering(Tests::Context& context)
{
	AutomaticFlightControl control = make_control();
	auto observation = nominal_observation();
	prime_and_engage(control, observation);
	observation.conditioned_roll_input_normalized =
		kTestStickInputNormalized;
	const auto steering = step(control, observation);
	TEST_EXPECT(
		context, steering.lateral_authority ==
			Core::Systems::AuthorityState::StickSteering);
	observation.conditioned_roll_input_normalized = 0.0;
	const auto recaptured = step(control, observation);
	TEST_EXPECT(
		context, recaptured.lateral_authority ==
			Core::Systems::AuthorityState::Automatic);
}
}

void run_automatic_flight_control_takeover_tests(Tests::Context& context)
{
	test_paddle_release_recaptures_each_vertical_mode(context);
	test_paddle_release_applies_each_lateral_mode_rule(context);
	test_path_modes_ignore_attitude_stick_steering(context);
	test_roll_attitude_hold_uses_stick_steering(context);
}
