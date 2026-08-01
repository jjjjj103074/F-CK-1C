#include "TestHarness.h"

#include "Common/Units.h"
#include "Core/Systems/FlightControlComputer/Autopilot/AutomaticFlightControl.h"

#include <cmath>
#include <iterator>

namespace
{
using Core::AutomaticFlightControlLateralMode;
using Core::AutomaticFlightControlReason;
using Core::AutomaticFlightControlVerticalMode;
using Core::CommandId;
using Core::Systems::AutomaticFlightControl;
using Core::Systems::AutomaticFlightControlObservation;

constexpr double kTolerance = 1e-9;
constexpr double kReferenceDtS = 0.02;
constexpr double kMetersPerSecondPerKnot = 0.5144444444444445;
constexpr double kNominalIasKts = 300.0;
constexpr double kNominalAltitudeM = 2000.0;
constexpr double kNominalMach = 0.7;
constexpr double kNominalHeadingRad = 1.0;
constexpr double kNominalPitchRad = 0.1;
constexpr double kNominalRollRad = -0.1;

double knots(double value)
{
	return value * kMetersPerSecondPerKnot;
}

AutomaticFlightControlObservation nominal_observation()
{
	AutomaticFlightControlObservation result;
	result.dt_s = kReferenceDtS;
	result.indicated_airspeed_mps = knots(kNominalIasKts);
	result.altitude_m = kNominalAltitudeM;
	result.mach = kNominalMach;
	result.heading_rad = kNominalHeadingRad;
	result.pitch_rad = kNominalPitchRad;
	result.roll_rad = kNominalRollRad;
	return result;
}

AutomaticFlightControl make_control(bool initial_wow = false)
{
	return AutomaticFlightControl(
		Core::Systems::fck1c_automatic_flight_control_config(),
		initial_wow);
}

void send(
	AutomaticFlightControl& control,
	CommandId id,
	double value = 1.0)
{
	control.handle_command({ id, value });
}

void prime_and_engage(
	AutomaticFlightControl& control,
	const AutomaticFlightControlObservation& observation)
{
	(void)control.step(observation);
	send(control, CommandId::EngageAutopilot);
	(void)control.step(observation);
}

void test_autopilot_engage_boundaries(Tests::Context& context)
{
	AutomaticFlightControl below = make_control();
	AutomaticFlightControlObservation observation = nominal_observation();
	observation.indicated_airspeed_mps = knots(239.0);
	prime_and_engage(below, observation);
	TEST_EXPECT(context, !below.snapshot().master_engaged);
	TEST_EXPECT(
		context,
		below.snapshot().autopilot_engage_rejection_reason ==
			AutomaticFlightControlReason::BelowMinimumIndicatedAirspeed);

	AutomaticFlightControl boundary = make_control();
	observation.indicated_airspeed_mps = knots(240.0);
	prime_and_engage(boundary, observation);
	TEST_EXPECT(context, boundary.snapshot().master_engaged);
	TEST_EXPECT(
		context,
		boundary.snapshot().vertical_mode ==
			AutomaticFlightControlVerticalMode::PitchHold);
	TEST_EXPECT(
		context,
		boundary.snapshot().lateral_mode ==
			AutomaticFlightControlLateralMode::HeadingHold);
}

void test_attitude_and_ground_engage_gates(Tests::Context& context)
{
	AutomaticFlightControlObservation observation = nominal_observation();
	observation.weight_on_wheels = true;
	AutomaticFlightControl ground = make_control();
	prime_and_engage(ground, observation);
	TEST_EXPECT(context, !ground.snapshot().master_engaged);
	TEST_EXPECT(
		context,
		ground.snapshot().autopilot_engage_rejection_reason ==
			AutomaticFlightControlReason::WeightOnWheels);

	observation.weight_on_wheels = false;
	AutomaticFlightControl roll = make_control();
	observation.roll_rad = Common::rad(45.01);
	prime_and_engage(roll, observation);
	TEST_EXPECT(context, !roll.snapshot().master_engaged);
	TEST_EXPECT(
		context,
		roll.snapshot().autopilot_engage_rejection_reason ==
			AutomaticFlightControlReason::RollLimit);

	AutomaticFlightControl pitch = make_control();
	observation.roll_rad = 0.0;
	observation.pitch_rad = Common::rad(-45.01);
	prime_and_engage(pitch, observation);
	TEST_EXPECT(context, !pitch.snapshot().master_engaged);
	TEST_EXPECT(
		context,
		pitch.snapshot().autopilot_engage_rejection_reason ==
			AutomaticFlightControlReason::PitchLimit);
}

void test_attitude_boundary_is_inclusive(Tests::Context& context)
{
	AutomaticFlightControlObservation observation = nominal_observation();
	observation.roll_rad = Common::rad(45.0);
	observation.pitch_rad = Common::rad(-45.0);
	AutomaticFlightControl control = make_control();
	prime_and_engage(control, observation);
	TEST_EXPECT(context, control.snapshot().master_engaged);
}

void test_engage_command_uses_next_tick_observation(
	Tests::Context& context)
{
	AutomaticFlightControl control = make_control();
	AutomaticFlightControlObservation observation = nominal_observation();
	(void)control.step(observation);
	send(control, CommandId::EngageAutopilot);
	TEST_EXPECT(context, !control.snapshot().master_engaged);
	observation.pitch_rad += Common::rad(1.0);
	observation.heading_rad += Common::rad(2.0);
	(void)control.step(observation);
	TEST_EXPECT(context, control.snapshot().master_engaged);
	TEST_EXPECT_NEAR(
		context,
		control.snapshot().target_pitch_rad,
		observation.pitch_rad,
		kTolerance);
	TEST_EXPECT_NEAR(
		context,
		control.snapshot().target_heading_rad,
		observation.heading_rad,
		kTolerance);
}

void test_pending_commands_preserve_arrival_order(Tests::Context& context)
{
	AutomaticFlightControl control = make_control();
	const AutomaticFlightControlObservation observation =
		nominal_observation();
	(void)control.step(observation);
	send(control, CommandId::EngageAutopilot);
	send(control, CommandId::SelectAutopilotAltitudeHold);
	TEST_EXPECT(context, !control.snapshot().master_engaged);
	(void)control.step(observation);
	TEST_EXPECT(context, control.snapshot().master_engaged);
	TEST_EXPECT(
		context,
		control.snapshot().vertical_mode ==
			AutomaticFlightControlVerticalMode::AltitudeHold);
	TEST_EXPECT_NEAR(
		context,
		control.snapshot().target_altitude_m,
		observation.altitude_m,
		kTolerance);
}

void test_default_capture_and_controller_direction(Tests::Context& context)
{
	AutomaticFlightControl control = make_control();
	AutomaticFlightControlObservation observation = nominal_observation();
	observation.roll_rad = 0.0;
	prime_and_engage(control, observation);
	TEST_EXPECT_NEAR(
		context,
		control.snapshot().target_pitch_rad,
		observation.pitch_rad,
		kTolerance);
	TEST_EXPECT_NEAR(
		context,
		control.snapshot().target_heading_rad,
		observation.heading_rad,
		kTolerance);
	observation.pitch_rad += Common::rad(2.0);
	observation.heading_rad += Common::rad(2.0);
	const auto& demand = control.step(observation);
	TEST_EXPECT(
		context,
		demand.pitch_attitude_reference_rad < observation.pitch_rad);
	TEST_EXPECT(context, demand.bank_angle_reference_rad < 0.0);
}

void test_pitch_controller_characterization(Tests::Context& context)
{
	AutomaticFlightControl control = make_control();
	AutomaticFlightControlObservation observation = nominal_observation();
	prime_and_engage(control, observation);
	observation.pitch_rad += 0.02;
	observation.legacy_pitch_damping_rate_rad_s = 0.03;
	TEST_EXPECT_NEAR(
		context,
		control.step(observation).pitch_attitude_reference_rad,
		kNominalPitchRad,
		kTolerance);
}

void test_vertical_speed_controller_characterization(
	Tests::Context& context)
{
	AutomaticFlightControl control = make_control();
	AutomaticFlightControlObservation observation = nominal_observation();
	observation.vertical_speed_mps = 4.0;
	prime_and_engage(control, observation);
	send(control, CommandId::SelectAutopilotVerticalSpeedHold);
	(void)control.step(observation);
	observation.vertical_speed_mps = 3.0;
	TEST_EXPECT_NEAR(
		context,
		control.step(observation).vertical_speed_reference_mps,
		4.0,
		kTolerance);
}

void test_heading_controller_characterization(Tests::Context& context)
{
	AutomaticFlightControl control = make_control();
	AutomaticFlightControlObservation observation = nominal_observation();
	observation.roll_rad = 0.0;
	prime_and_engage(control, observation);
	observation.heading_rad += 0.01;
	observation.legacy_heading_damping_rate_rad_s = 0.02;
	TEST_EXPECT_NEAR(
		context,
		control.step(observation).bank_angle_reference_rad,
		Common::rad(-0.4),
		Common::rad(0.01));
}

void test_vertical_modes_and_adjustments(Tests::Context& context)
{
	AutomaticFlightControl control = make_control();
	AutomaticFlightControlObservation observation = nominal_observation();
	observation.vertical_speed_mps = 4.0;
	prime_and_engage(control, observation);
	send(control, CommandId::SelectAutopilotVerticalSpeedHold);
	(void)control.step(observation);
	TEST_EXPECT_NEAR(
		context,
		control.snapshot().target_vertical_speed_mps,
		4.0,
		kTolerance);
	send(control, CommandId::IncreaseAutopilotVerticalReference);
	(void)control.step(observation);
	TEST_EXPECT_NEAR(
		context,
		control.snapshot().target_vertical_speed_mps,
		5.0,
		kTolerance);
	observation.vertical_speed_mps = 3.0;
	TEST_EXPECT(
		context,
		control.step(observation).vertical_speed_reference_mps >
			observation.vertical_speed_mps);

	send(control, CommandId::SelectAutopilotAltitudeHold);
	send(control, CommandId::IncreaseAutopilotVerticalReference);
	(void)control.step(observation);
	TEST_EXPECT_NEAR(
		context,
		control.snapshot().target_altitude_m,
		kNominalAltitudeM + 30.48,
		kTolerance);
	observation.altitude_m = kNominalAltitudeM - 100.0;
	TEST_EXPECT(
		context,
		control.step(observation).vertical_speed_reference_mps > 0.0);
}

void test_heading_wrap_and_navigation_placeholder(Tests::Context& context)
{
	AutomaticFlightControl control = make_control();
	AutomaticFlightControlObservation observation = nominal_observation();
	observation.heading_rad = Common::rad(359.5);
	prime_and_engage(control, observation);
	send(control, CommandId::IncreaseAutopilotLateralReference);
	(void)control.step(observation);
	TEST_EXPECT_NEAR(
		context,
		control.snapshot().target_heading_rad,
		Common::rad(0.5),
		kTolerance);
	send(control, CommandId::SelectAutopilotNavigationTrack);
	(void)control.step(observation);
	TEST_EXPECT(
		context,
		control.snapshot().lateral_mode ==
			AutomaticFlightControlLateralMode::NavigationTrack);
	observation.roll_rad = Common::rad(20.0);
	TEST_EXPECT_NEAR(
		context,
		control.step(observation).bank_angle_reference_rad,
		0.0,
		kTolerance);
}

void test_bypass_freeze_and_recapture(Tests::Context& context)
{
	AutomaticFlightControl control = make_control();
	AutomaticFlightControlObservation observation = nominal_observation();
	prime_and_engage(control, observation);
	send(control, CommandId::SetAutopilotBypass);
	(void)control.step(observation);
	observation.pitch_rad += Common::rad(2.0);
	observation.heading_rad += Common::rad(2.0);
	const auto& bypass_demand = control.step(observation);
	TEST_EXPECT(context, control.snapshot().bypass_active);
	TEST_EXPECT_NEAR(
		context, bypass_demand.pitch_attitude_reference_rad, 0.0, kTolerance);
	TEST_EXPECT_NEAR(
		context, bypass_demand.bank_angle_reference_rad, 0.0, kTolerance);
	send(control, CommandId::SetAutopilotBypass, 0.0);
	(void)control.step(observation);
	TEST_EXPECT(context, !control.snapshot().bypass_active);
	TEST_EXPECT_NEAR(
		context,
		control.snapshot().target_pitch_rad,
		observation.pitch_rad,
		kTolerance);
	TEST_EXPECT_NEAR(
		context,
		control.snapshot().target_heading_rad,
		observation.heading_rad,
		kTolerance);
}

void test_bypass_always_recaptures_on_release(Tests::Context& context)
{
	AutomaticFlightControl control = make_control();
	AutomaticFlightControlObservation observation = nominal_observation();
	prime_and_engage(control, observation);
	send(control, CommandId::SetAutopilotBypass);
	(void)control.step(observation);
	observation.pitch_rad += Common::rad(0.5);
	observation.heading_rad += Common::rad(0.5);
	(void)control.step(observation);
	send(control, CommandId::SetAutopilotBypass, 0.0);
	(void)control.step(observation);
	TEST_EXPECT_NEAR(
		context,
		control.snapshot().target_pitch_rad,
		observation.pitch_rad,
		kTolerance);
	TEST_EXPECT_NEAR(
		context,
		control.snapshot().target_heading_rad,
		observation.heading_rad,
		kTolerance);
}

void test_pitch_hold_stick_steering_is_axis_specific(
	Tests::Context& context)
{
	AutomaticFlightControl control = make_control();
	AutomaticFlightControlObservation observation = nominal_observation();
	prime_and_engage(control, observation);
	observation.pitch_rad += Common::rad(3.0);
	observation.conditioned_pitch_input_normalized = 0.2;
	const auto steering = control.step(observation);
	TEST_EXPECT(
		context,
		steering.longitudinal_authority ==
			Core::Systems::AuthorityState::StickSteering);
	TEST_EXPECT(
		context,
		steering.lateral_authority ==
			Core::Systems::AuthorityState::Automatic);
	TEST_EXPECT_NEAR(
		context,
		control.snapshot().target_pitch_rad,
		observation.pitch_rad,
		kTolerance);
	observation.pitch_rad += Common::rad(1.0);
	observation.conditioned_pitch_input_normalized = 0.0;
	const auto recaptured = control.step(observation);
	TEST_EXPECT(
		context,
		recaptured.longitudinal_authority ==
			Core::Systems::AuthorityState::Automatic);
	TEST_EXPECT_NEAR(
		context,
		control.snapshot().target_pitch_rad,
		observation.pitch_rad,
		kTolerance);
}

void test_path_modes_do_not_use_attitude_stick_steering(
	Tests::Context& context)
{
	AutomaticFlightControl control = make_control();
	AutomaticFlightControlObservation observation = nominal_observation();
	prime_and_engage(control, observation);
	send(control, CommandId::SelectAutopilotVerticalSpeedHold);
	observation.conditioned_pitch_input_normalized = 0.5;
	const auto demand = control.step(observation);
	TEST_EXPECT(
		context,
		demand.longitudinal_authority ==
			Core::Systems::AuthorityState::Automatic);
}

void test_disconnect_guards(Tests::Context& context)
{
	AutomaticFlightControl control = make_control();
	AutomaticFlightControlObservation observation = nominal_observation();
	prime_and_engage(control, observation);
	observation.indicated_airspeed_mps = knots(239.0);
	(void)control.step(observation);
	TEST_EXPECT(context, !control.snapshot().master_engaged);
	TEST_EXPECT(
		context,
		control.snapshot().autopilot_disengage_reason ==
			AutomaticFlightControlReason::BelowMinimumIndicatedAirspeed);

	observation = nominal_observation();
	(void)control.step(observation);
	send(control, CommandId::EngageAutoThrottle);
	(void)control.step(observation);
	TEST_EXPECT(context, control.snapshot().auto_throttle_engaged);
	observation.mach = 1.0;
	(void)control.step(observation);
	TEST_EXPECT(context, control.snapshot().auto_throttle_engaged);
	observation.mach = 1.001;
	(void)control.step(observation);
	TEST_EXPECT(context, !control.snapshot().auto_throttle_engaged);
	TEST_EXPECT(
		context,
		control.snapshot().auto_throttle_disengage_reason ==
			AutomaticFlightControlReason::MachLimit);
}

void test_auto_throttle_engage_gates(Tests::Context& context)
{
	AutomaticFlightControlObservation observation = nominal_observation();
	observation.mach = 0.95;
	AutomaticFlightControl boundary = make_control();
	(void)boundary.step(observation);
	send(boundary, CommandId::EngageAutoThrottle);
	(void)boundary.step(observation);
	TEST_EXPECT(context, boundary.snapshot().auto_throttle_engaged);

	observation.mach = 0.951;
	AutomaticFlightControl fast = make_control();
	(void)fast.step(observation);
	send(fast, CommandId::EngageAutoThrottle);
	(void)fast.step(observation);
	TEST_EXPECT(context, !fast.snapshot().auto_throttle_engaged);
	TEST_EXPECT(
		context,
		fast.snapshot().auto_throttle_engage_rejection_reason ==
			AutomaticFlightControlReason::MachLimit);

	observation = nominal_observation();
	observation.weight_on_wheels = true;
	AutomaticFlightControl ground = make_control();
	(void)ground.step(observation);
	send(ground, CommandId::EngageAutoThrottle);
	(void)ground.step(observation);
	TEST_EXPECT(context, !ground.snapshot().auto_throttle_engaged);
	TEST_EXPECT(
		context,
		ground.snapshot().auto_throttle_engage_rejection_reason ==
			AutomaticFlightControlReason::WeightOnWheels);
}

double run_mach_guard(const double* steps, std::size_t count)
{
	AutomaticFlightControl control = make_control();
	AutomaticFlightControlObservation observation = nominal_observation();
	(void)control.step(observation);
	send(control, CommandId::EngageAutoThrottle);
	(void)control.step(observation);
	observation.mach = 0.96;
	for (std::size_t index = 0; index < count; ++index)
	{
		observation.dt_s = steps[index];
		(void)control.step(observation);
	}
	return control.snapshot().throttle_command_normalized;
}

void test_mach_guard_uses_elapsed_time(Tests::Context& context)
{
	const double fixed[] = { 0.02 };
	const double irregular[] = { 0.003, 0.007, 0.01 };
	TEST_EXPECT_NEAR(
		context,
		run_mach_guard(fixed, std::size(fixed)),
		run_mach_guard(irregular, std::size(irregular)),
		kTolerance);
}

void test_auto_throttle_target_limits(Tests::Context& context)
{
	AutomaticFlightControl control = make_control();
	AutomaticFlightControlObservation observation = nominal_observation();
	observation.indicated_airspeed_mps = knots(548.0);
	(void)control.step(observation);
	send(control, CommandId::EngageAutoThrottle);
	send(control, CommandId::IncreaseAutopilotSpeed);
	(void)control.step(observation);
	TEST_EXPECT_NEAR(
		context,
		control.snapshot().target_speed_mps,
		knots(550.0),
		kTolerance);
	send(control, CommandId::IncreaseAutopilotSpeed);
	(void)control.step(observation);
	TEST_EXPECT_NEAR(
		context,
		control.snapshot().target_speed_mps,
		knots(550.0),
		kTolerance);

	AutomaticFlightControl minimum = make_control();
	observation.indicated_airspeed_mps = knots(202.0);
	(void)minimum.step(observation);
	send(minimum, CommandId::EngageAutoThrottle);
	send(minimum, CommandId::DecreaseAutopilotSpeed);
	(void)minimum.step(observation);
	TEST_EXPECT_NEAR(
		context,
		minimum.snapshot().target_speed_mps,
		knots(200.0),
		kTolerance);
	send(minimum, CommandId::DecreaseAutopilotSpeed);
	(void)minimum.step(observation);
	TEST_EXPECT_NEAR(
		context,
		minimum.snapshot().target_speed_mps,
		knots(200.0),
		kTolerance);
}

void test_auto_throttle_controller_characterization(
	Tests::Context& context)
{
	AutomaticFlightControl control = make_control();
	AutomaticFlightControlObservation observation = nominal_observation();
	(void)control.step(observation);
	send(control, CommandId::EngageAutoThrottle);
	(void)control.step(observation);
	observation.indicated_airspeed_mps -= 1.0;
	TEST_EXPECT_NEAR(
		context,
		control.step(observation).experimental_throttle_normalized,
		0.51506,
		kTolerance);
}

void test_commanded_disconnect_resets_both_channels(
	Tests::Context& context)
{
	AutomaticFlightControl control = make_control();
	const AutomaticFlightControlObservation observation =
		nominal_observation();
	prime_and_engage(control, observation);
	send(control, CommandId::EngageAutoThrottle);
	send(control, CommandId::DisengageAutopilot);
	(void)control.step(observation);
	TEST_EXPECT(context, !control.snapshot().master_engaged);
	TEST_EXPECT(context, !control.snapshot().auto_throttle_engaged);
	TEST_EXPECT(
		context,
		control.snapshot().autopilot_disengage_reason ==
			AutomaticFlightControlReason::Commanded);
	TEST_EXPECT(
		context,
		control.snapshot().auto_throttle_disengage_reason ==
			AutomaticFlightControlReason::Commanded);
}

void test_sustained_failure_changes_mode_on_next_tick(
	Tests::Context& context)
{
	AutomaticFlightControl control = make_control();
	const AutomaticFlightControlObservation observation = nominal_observation();
	prime_and_engage(control, observation);
	send(control, CommandId::SelectAutopilotVerticalSpeedHold);
	(void)control.step(observation);
	Core::Systems::AutopilotModeMonitorObservation monitor;
	monitor.dt_s = 1.0;
	monitor.vertical_active = true;
	monitor.vertical_type =
		Core::Systems::VerticalGuidanceReferenceType::VerticalSpeed;
	monitor.vertical_tracking_error = 6.0;
	control.observe_control_result(monitor);
	(void)control.step(observation);
	control.observe_control_result(monitor);
	TEST_EXPECT(
		context,
		control.snapshot().vertical_mode ==
			AutomaticFlightControlVerticalMode::VerticalSpeedHold);
	(void)control.step(observation);
	TEST_EXPECT(
		context,
		control.snapshot().vertical_mode ==
			AutomaticFlightControlVerticalMode::Off);
	TEST_EXPECT(
		context,
		control.snapshot().lateral_mode ==
			AutomaticFlightControlLateralMode::HeadingHold);
	TEST_EXPECT(context, control.snapshot().vertical_degraded);
}
}

void run_automatic_flight_control_tests(Tests::Context& context)
{
	test_autopilot_engage_boundaries(context);
	test_attitude_and_ground_engage_gates(context);
	test_attitude_boundary_is_inclusive(context);
	test_engage_command_uses_next_tick_observation(context);
	test_pending_commands_preserve_arrival_order(context);
	test_default_capture_and_controller_direction(context);
	test_pitch_controller_characterization(context);
	test_vertical_speed_controller_characterization(context);
	// The legacy normalized-command plant is replaced by the Phase 9
	// physical-reference closed-loop harness.
	test_heading_controller_characterization(context);
	test_vertical_modes_and_adjustments(context);
	test_heading_wrap_and_navigation_placeholder(context);
	test_bypass_freeze_and_recapture(context);
	test_bypass_always_recaptures_on_release(context);
	test_pitch_hold_stick_steering_is_axis_specific(context);
	test_path_modes_do_not_use_attitude_stick_steering(context);
	test_disconnect_guards(context);
	test_auto_throttle_engage_gates(context);
	test_mach_guard_uses_elapsed_time(context);
	test_auto_throttle_target_limits(context);
	test_auto_throttle_controller_characterization(context);
	test_commanded_disconnect_resets_both_channels(context);
	test_sustained_failure_changes_mode_on_next_tick(context);
}
