#include "TestHarness.h"
#include "AutomaticFlightControlTestSupport.h"

#include "Common/Units.h"

#include <cmath>
#include <iterator>

namespace
{
using namespace AutomaticFlightControlTestSupport;

constexpr double kNegativeHeadingCaptureDeg = -10.0;
constexpr int kCanonicalCapturedHeadingDeg = 350;
constexpr int kCanonicalAdjustedHeadingDeg = 351;
constexpr double kHardEnvelopeViolationRollDeg = 60.1;
constexpr double kIntegralTestDtS = 0.1;
constexpr int kIntegralTestHeadingErrorDeg = 1;
constexpr int kConstrainedIntegralTicks = 20;

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
			AutomaticFlightControlVerticalMode::PitchAttitudeHold);
	TEST_EXPECT(
		context,
		boundary.snapshot().lateral_mode ==
			AutomaticFlightControlLateralMode::RollAttitudeHold);
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

void test_heading_select_requires_magnetic_heading(
	Tests::Context& context)
{
	AutomaticFlightControl control = make_control();
	auto observation = nominal_observation();
	observation.magnetic_heading_available = false;
	(void)step(control, observation);
	send(control, CommandId::SelectAutopilotHeadingSelect);
	(void)step(control, observation);
	send(control, CommandId::EngageAutopilot);
	(void)step(control, observation);
	TEST_EXPECT(context, !control.snapshot().master_engaged);
	TEST_EXPECT(
		context,
		control.snapshot().autopilot_engage_rejection_reason ==
			AutomaticFlightControlReason::MagneticHeadingUnavailable);
}

void test_heading_select_releases_lateral_channel_when_heading_is_lost(
	Tests::Context& context)
{
	AutomaticFlightControl control = make_control();
	auto observation = nominal_observation();
	(void)step(control, observation);
	send(control, CommandId::SelectAutopilotHeadingSelect);
	prime_and_engage(control, observation);
	observation.magnetic_heading_available = false;
	(void)step(control, observation);
	TEST_EXPECT(context, control.snapshot().master_engaged);
	TEST_EXPECT(
		context,
		control.snapshot().lateral_mode ==
			AutomaticFlightControlLateralMode::Off);
	TEST_EXPECT(context, control.snapshot().lateral_degraded);
	TEST_EXPECT(
		context,
		control.snapshot().degradation_reason ==
			Core::FlightControlDegradationReason::MagneticHeadingUnavailable);

	observation.magnetic_heading_available = true;
	send(control, CommandId::SelectAutopilotHeadingSelect);
	(void)step(control, observation);
	TEST_EXPECT(
		context,
		control.snapshot().lateral_mode ==
			AutomaticFlightControlLateralMode::HeadingSelect);
	TEST_EXPECT(context, !control.snapshot().lateral_degraded);
}

void test_altitude_hold_requires_pressure_altitude(
	Tests::Context& context)
{
	AutomaticFlightControl control = make_control();
	auto observation = nominal_observation();
	observation.pressure_altitude_available = false;
	(void)step(control, observation);
	send(control, CommandId::SelectAutopilotAltitudeHold);
	send(control, CommandId::EngageAutopilot);
	(void)step(control, observation);
	TEST_EXPECT(context, !control.snapshot().master_engaged);
	TEST_EXPECT(context,
		control.snapshot().autopilot_engage_rejection_reason ==
			AutomaticFlightControlReason::PressureAltitudeUnavailable);
}

void test_altitude_hold_releases_vertical_channel_when_altitude_is_lost(
	Tests::Context& context)
{
	AutomaticFlightControl control = make_control();
	auto observation = nominal_observation();
	(void)step(control, observation);
	send(control, CommandId::SelectAutopilotAltitudeHold);
	prime_and_engage(control, observation);
	observation.pressure_altitude_available = false;
	(void)step(control, observation);
	TEST_EXPECT(context, control.snapshot().master_engaged);
	TEST_EXPECT(context,
		control.snapshot().vertical_mode ==
			AutomaticFlightControlVerticalMode::Off);
	TEST_EXPECT(context, control.snapshot().vertical_degraded);
	TEST_EXPECT(context,
		control.snapshot().degradation_reason ==
			Core::FlightControlDegradationReason::PressureAltitudeUnavailable);
}

void test_engage_command_uses_next_tick_observation(
	Tests::Context& context)
{
	AutomaticFlightControl control = make_control();
	AutomaticFlightControlObservation observation = nominal_observation();
	(void)step(control, observation);
	send(control, CommandId::EngageAutopilot);
	TEST_EXPECT(context, !control.snapshot().master_engaged);
	observation.pitch_rad += Common::rad(1.0);
	observation.roll_rad += Common::rad(2.0);
	(void)step(control, observation);
	TEST_EXPECT(context, control.snapshot().master_engaged);
	TEST_EXPECT_NEAR(
		context,
		control.snapshot().target_pitch_rad,
		observation.pitch_rad,
		kTolerance);
	TEST_EXPECT(
		context,
		control.snapshot().bank_angle_reference_rad < 0.0);
}

void test_pending_commands_preserve_arrival_order(Tests::Context& context)
{
	AutomaticFlightControl control = make_control();
	const AutomaticFlightControlObservation observation =
		nominal_observation();
	(void)step(control, observation);
	send(control, CommandId::SelectAutopilotAltitudeHold);
	send(control, CommandId::EngageAutopilot);
	TEST_EXPECT(context, !control.snapshot().master_engaged);
	(void)step(control, observation);
	TEST_EXPECT(context, control.snapshot().master_engaged);
	TEST_EXPECT(
		context,
		control.snapshot().vertical_mode ==
			AutomaticFlightControlVerticalMode::AltitudeHold);
	TEST_EXPECT_NEAR(
		context,
		control.snapshot().target_altitude_ft,
		observation.pressure_altitude_ft,
		kTolerance);
}

void test_f16ab_mode_switches_preselect_without_engaging(
	Tests::Context& context)
{
	AutomaticFlightControl control = make_control();
	const AutomaticFlightControlObservation observation =
		nominal_observation();
	(void)step(control, observation);
	send(control, CommandId::SelectAutopilotAltitudeHold);
	send(control, CommandId::SelectAutopilotHeadingSelect);
	(void)step(control, observation);
	TEST_EXPECT(context, !control.snapshot().master_engaged);
	TEST_EXPECT(
		context,
		control.snapshot().vertical_mode ==
			AutomaticFlightControlVerticalMode::Off);
	TEST_EXPECT(
		context,
		control.snapshot().lateral_mode ==
			AutomaticFlightControlLateralMode::Off);
	send(control, CommandId::EngageAutopilot);
	(void)step(control, observation);
	TEST_EXPECT(
		context,
		control.snapshot().vertical_mode ==
			AutomaticFlightControlVerticalMode::AltitudeHold);
	TEST_EXPECT(
		context,
		control.snapshot().lateral_mode ==
			AutomaticFlightControlLateralMode::HeadingSelect);
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
		control.snapshot().bank_angle_reference_rad,
		observation.roll_rad,
		kTolerance);
	observation.pitch_rad += Common::rad(2.0);
	observation.roll_rad += Common::rad(2.0);
	const auto& demand = step(control, observation);
	TEST_EXPECT(
		context,
		demand.pitch_attitude_reference_rad < observation.pitch_rad);
	TEST_EXPECT(
		context,
		demand.bank_angle_reference_rad < observation.roll_rad);
}

void test_pitch_controller_characterization(Tests::Context& context)
{
	AutomaticFlightControl control = make_control();
	AutomaticFlightControlObservation observation = nominal_observation();
	prime_and_engage(control, observation);
	observation.pitch_rad += 0.02;
	TEST_EXPECT_NEAR(
		context,
		step(control, observation).pitch_attitude_reference_rad,
		kNominalPitchRad,
		kTolerance);
}

void test_heading_controller_characterization(Tests::Context& context)
{
	AutomaticFlightControl control = make_control();
	AutomaticFlightControlObservation observation = nominal_observation();
	observation.magnetic_heading_deg = 0.0;
	observation.roll_rad = 0.0;
	(void)step(control, observation);
	send(control, CommandId::SelectAutopilotHeadingSelect);
	prime_and_engage(control, observation);
	observation.magnetic_heading_deg += Common::deg(0.01);
	TEST_EXPECT_NEAR(
		context,
		step(control, observation).bank_angle_reference_rad,
		Common::rad(-0.4),
		Common::rad(0.01));
}

void test_altitude_hold_captures_current_altitude(Tests::Context& context)
{
	AutomaticFlightControl control = make_control();
	AutomaticFlightControlObservation observation = nominal_observation();
	prime_and_engage(control, observation);
	send(control, CommandId::SelectAutopilotAltitudeHold);
	(void)step(control, observation);
	TEST_EXPECT_NEAR(
		context,
		control.snapshot().target_altitude_ft,
		kNominalAltitudeFt,
		kTolerance);
	observation.pressure_altitude_ft = kNominalAltitudeFt - 100.0;
	send(control, CommandId::SelectAutopilotAltitudeHold);
	TEST_EXPECT(
		context,
		step(control, observation).vertical_speed_reference_ft_s > 0.0);
	TEST_EXPECT_NEAR(
		context,
		control.snapshot().target_altitude_ft,
		kNominalAltitudeFt,
		kTolerance);
}

void test_heading_wrap(Tests::Context& context)
{
	AutomaticFlightControl control = make_control();
	AutomaticFlightControlObservation observation = nominal_observation();
	observation.magnetic_heading_deg = 359.0;
	(void)step(control, observation);
	send(control, CommandId::SelectAutopilotHeadingSelect);
	send(control, CommandId::IncreaseAutopilotHeadingSelect);
	(void)step(control, observation);
	TEST_EXPECT(context, control.snapshot().target_heading_deg == 0);
}

void test_negative_heading_capture_uses_canonical_display_range(
	Tests::Context& context)
{
	AutomaticFlightControl control = make_control();
	AutomaticFlightControlObservation observation = nominal_observation();
	observation.magnetic_heading_deg = kNegativeHeadingCaptureDeg;
	(void)step(control, observation);
	send(control, CommandId::SelectAutopilotHeadingSelect);
	(void)step(control, observation);
	TEST_EXPECT(
		context,
		control.snapshot().target_heading_deg ==
			kCanonicalCapturedHeadingDeg);
	send(control, CommandId::IncreaseAutopilotHeadingSelect);
	(void)step(control, observation);
	TEST_EXPECT(
		context,
		control.snapshot().target_heading_deg ==
			kCanonicalAdjustedHeadingDeg);
	observation.magnetic_heading_deg = 90.0;
	send(control, CommandId::SelectAutopilotHeadingSelect);
	(void)step(control, observation);
	TEST_EXPECT(
		context,
		control.snapshot().target_heading_deg ==
			kCanonicalAdjustedHeadingDeg);
}

void test_lateral_constraint_freezes_integral(Tests::Context& context)
{
	auto config = Core::Systems::fck1c_automatic_flight_control_config();
	config.heading_kp = 0.0;
	config.heading_ki = 1.0;
	config.bank_limit_rad = 1.0;
	config.roll_reference_rate_rad_s = 1.0;
	config.heading_select_step_deg = kIntegralTestHeadingErrorDeg;
	AutomaticFlightControl constrained(config, false);
	AutomaticFlightControl fresh(config, false);
	auto observation = nominal_observation();
	observation.dt_s = kIntegralTestDtS;
	observation.magnetic_heading_deg = 0.0;
	observation.roll_rad = 0.0;
	for (AutomaticFlightControl* control : { &constrained, &fresh })
	{
		(void)step(*control, observation);
		send(*control, CommandId::SelectAutopilotHeadingSelect);
		send(*control, CommandId::IncreaseAutopilotHeadingSelect);
		send(*control, CommandId::EngageAutopilot);
		(void)step(*control, observation);
	}
	Core::Systems::FlightControlCommandMonitorInput monitor;
	monitor.dt_s = observation.dt_s;
	monitor.lateral_active = true;
	monitor.constraint.lateral_constrained = true;
	monitor.constraint.reason =
		Core::Systems::ConstraintReason::LateralConstrainedByVerticalAuthority;
	for (int tick = 0; tick < kConstrainedIntegralTicks; ++tick)
	{
		constrained.observe_control_result(monitor);
		(void)step(constrained, observation);
	}
	monitor.constraint = {};
	constrained.observe_control_result(monitor);
	const double recovered =
		step(constrained, observation).bank_angle_reference_rad;
	const double first_unconstrained =
		step(fresh, observation).bank_angle_reference_rad;
	TEST_EXPECT_NEAR(
		context, recovered, first_unconstrained, kTolerance);
}

void test_bypass_freeze_and_recapture(Tests::Context& context)
{
	AutomaticFlightControl control = make_control();
	AutomaticFlightControlObservation observation = nominal_observation();
	prime_and_engage(control, observation);
	send(control, CommandId::SetAutopilotBypass);
	(void)step(control, observation);
	observation.pitch_rad += Common::rad(2.0);
	observation.roll_rad += Common::rad(2.0);
	const auto& bypass_demand = step(control, observation);
	TEST_EXPECT(context, control.snapshot().bypass_active);
	TEST_EXPECT_NEAR(
		context, bypass_demand.pitch_attitude_reference_rad, 0.0, kTolerance);
	TEST_EXPECT_NEAR(
		context, bypass_demand.bank_angle_reference_rad, 0.0, kTolerance);
	send(control, CommandId::SetAutopilotBypass, 0.0);
	(void)step(control, observation);
	TEST_EXPECT(context, !control.snapshot().bypass_active);
	TEST_EXPECT_NEAR(
		context,
		control.snapshot().target_pitch_rad,
		observation.pitch_rad,
		kTolerance);
	TEST_EXPECT(
		context,
		control.snapshot().bank_angle_reference_rad < 0.0);
}

void test_bypass_always_recaptures_on_release(Tests::Context& context)
{
	AutomaticFlightControl control = make_control();
	AutomaticFlightControlObservation observation = nominal_observation();
	prime_and_engage(control, observation);
	send(control, CommandId::SetAutopilotBypass);
	(void)step(control, observation);
	observation.pitch_rad += Common::rad(0.5);
	observation.roll_rad += Common::rad(0.5);
	(void)step(control, observation);
	send(control, CommandId::SetAutopilotBypass, 0.0);
	(void)step(control, observation);
	TEST_EXPECT_NEAR(
		context,
		control.snapshot().target_pitch_rad,
		observation.pitch_rad,
		kTolerance);
	TEST_EXPECT(
		context,
		control.snapshot().bank_angle_reference_rad < 0.0);
}

void test_pitch_hold_stick_steering_is_axis_specific(
	Tests::Context& context)
{
	AutomaticFlightControl control = make_control();
	AutomaticFlightControlObservation observation = nominal_observation();
	prime_and_engage(control, observation);
	observation.pitch_rad += Common::rad(3.0);
	observation.conditioned_pitch_input_normalized = 0.2;
	const auto steering = step(control, observation);
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
	const auto recaptured = step(control, observation);
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
	send(control, CommandId::SelectAutopilotAltitudeHold);
	observation.conditioned_pitch_input_normalized = 0.5;
	const auto demand = step(control, observation);
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
	(void)step(control, observation);
	TEST_EXPECT(context, !control.snapshot().master_engaged);
	TEST_EXPECT(
		context,
		control.snapshot().autopilot_disengage_reason ==
			AutomaticFlightControlReason::BelowMinimumIndicatedAirspeed);

	observation = nominal_observation();
	(void)step(control, observation);
	send(control, CommandId::EngageAutoThrottle);
	(void)step(control, observation);
	TEST_EXPECT(context, control.snapshot().auto_throttle_engaged);
	observation.mach = 1.0;
	(void)step(control, observation);
	TEST_EXPECT(context, control.snapshot().auto_throttle_engaged);
	observation.mach = 1.001;
	(void)step(control, observation);
	TEST_EXPECT(context, !control.snapshot().auto_throttle_engaged);
	TEST_EXPECT(
		context,
		control.snapshot().auto_throttle_disengage_reason ==
			AutomaticFlightControlReason::MachLimit);
}

void test_hard_envelope_disconnects_engaged_autopilot(
	Tests::Context& context)
{
	AutomaticFlightControl control = make_control();
	AutomaticFlightControlObservation observation = nominal_observation();
	observation.roll_rad = 0.0;
	prime_and_engage(control, observation);
	observation.roll_rad = Common::rad(kHardEnvelopeViolationRollDeg);
	(void)step(control, observation);
	TEST_EXPECT(context, !control.snapshot().master_engaged);
	TEST_EXPECT(
		context,
		control.snapshot().autopilot_disengage_reason ==
			AutomaticFlightControlReason::RollLimit);
	TEST_EXPECT(
		context,
		control.snapshot().disconnect_reason ==
			Core::FlightControlDisconnectReason::SafetyCondition);
}

}

void run_automatic_flight_control_tests(Tests::Context& context)
{
	test_autopilot_engage_boundaries(context);
	test_attitude_and_ground_engage_gates(context);
	test_attitude_boundary_is_inclusive(context);
	test_heading_select_requires_magnetic_heading(context);
	test_heading_select_releases_lateral_channel_when_heading_is_lost(context);
	test_altitude_hold_requires_pressure_altitude(context);
	test_altitude_hold_releases_vertical_channel_when_altitude_is_lost(context);
	test_engage_command_uses_next_tick_observation(context);
	test_pending_commands_preserve_arrival_order(context);
	test_f16ab_mode_switches_preselect_without_engaging(context);
	test_default_capture_and_controller_direction(context);
	test_pitch_controller_characterization(context);
	// The legacy normalized-command plant is replaced by the Phase 9
	// physical-reference closed-loop harness.
	test_heading_controller_characterization(context);
	test_altitude_hold_captures_current_altitude(context);
	test_heading_wrap(context);
	test_negative_heading_capture_uses_canonical_display_range(context);
	test_lateral_constraint_freezes_integral(context);
	test_bypass_freeze_and_recapture(context);
	test_bypass_always_recaptures_on_release(context);
	test_pitch_hold_stick_steering_is_axis_specific(context);
	test_path_modes_do_not_use_attitude_stick_steering(context);
	test_disconnect_guards(context);
	test_hard_envelope_disconnects_engaged_autopilot(context);
}
