#include "TestHarness.h"
#include "AutomaticFlightControlTestSupport.h"

#include "Common/Units.h"

namespace
{
using namespace AutomaticFlightControlTestSupport;

constexpr double kPaddlePitchChangeDeg = 3.0;
constexpr double kPaddleVerticalSpeedChangeMps = 2.0;
constexpr double kPaddleAltitudeChangeM = 100.0;
constexpr double kPaddleHeadingChangeDeg = 5.0;
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
	case AutomaticFlightControlVerticalMode::PitchHold:
		TEST_EXPECT_NEAR(
			context, snapshot.target_pitch_rad,
			observation.pitch_rad, kTolerance);
		break;
	case AutomaticFlightControlVerticalMode::VerticalSpeedHold:
		TEST_EXPECT_NEAR(
			context, snapshot.target_vertical_speed_mps,
			observation.vertical_speed_mps, kTolerance);
		break;
	case AutomaticFlightControlVerticalMode::AltitudeHold:
		TEST_EXPECT_NEAR(
			context, snapshot.target_altitude_m,
			observation.altitude_m, kTolerance);
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
		{ CommandId::SelectAutopilotPitchHold,
			AutomaticFlightControlVerticalMode::PitchHold },
		{ CommandId::SelectAutopilotVerticalSpeedHold,
			AutomaticFlightControlVerticalMode::VerticalSpeedHold },
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
		observation.vertical_speed_mps += kPaddleVerticalSpeedChangeMps;
		observation.altitude_m += kPaddleAltitudeChangeM;
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
	AutomaticFlightControl heading_hold = make_control();
	auto observation = nominal_observation();
	prime_and_engage(heading_hold, observation);
	send(heading_hold, CommandId::SetAutopilotBypass);
	(void)step(heading_hold, observation);
	observation.heading_rad += Common::rad(kPaddleHeadingChangeDeg);
	send(
		heading_hold, CommandId::SetAutopilotBypass,
		kReleasedCommandValue);
	(void)step(heading_hold, observation);
	TEST_EXPECT_NEAR(
		context, heading_hold.snapshot().target_heading_rad,
		observation.heading_rad, kTolerance);

	AutomaticFlightControl heading_select = make_control();
	observation = nominal_observation();
	prime_and_engage(heading_select, observation);
	send(heading_select, CommandId::SelectAutopilotHeading);
	send(heading_select, CommandId::IncreaseAutopilotLateralReference);
	(void)step(heading_select, observation);
	const double selected_heading =
		heading_select.snapshot().target_heading_rad;
	send(heading_select, CommandId::SetAutopilotBypass);
	(void)step(heading_select, observation);
	observation.heading_rad += Common::rad(kPaddleHeadingChangeDeg);
	send(
		heading_select, CommandId::SetAutopilotBypass,
		kReleasedCommandValue);
	(void)step(heading_select, observation);
	TEST_EXPECT_NEAR(
		context, heading_select.snapshot().target_heading_rad,
		selected_heading, kTolerance);
}

void test_path_modes_ignore_attitude_stick_steering(
	Tests::Context& context)
{
	const CommandId vertical_modes[] = {
		CommandId::SelectAutopilotVerticalSpeedHold,
		CommandId::SelectAutopilotAltitudeHold
	};
	for (CommandId mode : vertical_modes)
	{
		AutomaticFlightControl control = make_control();
		auto observation = nominal_observation();
		prime_and_engage(control, observation);
		send(control, mode);
		observation.conditioned_pitch_input_normalized =
			kTestStickInputNormalized;
		const auto reference = step(control, observation);
		TEST_EXPECT(
			context, reference.longitudinal_authority ==
				Core::Systems::AuthorityState::Automatic);
	}
	const CommandId lateral_modes[] = {
		CommandId::SelectAutopilotHeadingHold,
		CommandId::SelectAutopilotHeading
	};
	for (CommandId mode : lateral_modes)
	{
		AutomaticFlightControl control = make_control();
		auto observation = nominal_observation();
		prime_and_engage(control, observation);
		send(control, mode);
		observation.conditioned_roll_input_normalized =
			kTestStickInputNormalized;
		const auto reference = step(control, observation);
		TEST_EXPECT(
			context, reference.lateral_authority ==
				Core::Systems::AuthorityState::Automatic);
	}
}
}

void run_automatic_flight_control_takeover_tests(Tests::Context& context)
{
	test_paddle_release_recaptures_each_vertical_mode(context);
	test_paddle_release_applies_each_lateral_mode_rule(context);
	test_path_modes_ignore_attitude_stick_steering(context);
}
