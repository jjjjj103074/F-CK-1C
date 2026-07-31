#include "TestHarness.h"

#include "Common/Units.h"
#include "Core/Systems/FlightControlComputer/AutomaticFlightControl.h"

#include <algorithm>
#include <array>
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
constexpr double kDcsFrameDtS = 0.006;
// Derived from the 2026-07-31 DCS trace that exposed the ALT-hold oscillation.
constexpr double kMeasuredFbwResponseDelayS = 0.55;
constexpr double kMeasuredGPerPitchCommand = 4.1;
constexpr double kGravityMps2 = 9.81;
constexpr double kAltitudeCaptureInitialVerticalSpeedMps = -3.0;
constexpr double kAltitudeCaptureDurationS = 30.0;
constexpr double kMaximumCaptureErrorM = 3.0;
constexpr double kMaximumSettledVerticalSpeedMps = 0.5;
constexpr int kMaximumCaptureErrorCrossings = 5;
constexpr double kPitchCommandSaturationThreshold = 0.599;
constexpr std::size_t kFbwResponseDelayFrames =
	static_cast<std::size_t>(
		kMeasuredFbwResponseDelayS / kDcsFrameDtS + 0.5);

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

struct VerticalPlant
{
	double altitude_m = kNominalAltitudeM;
	double vertical_speed_mps = kAltitudeCaptureInitialVerticalSpeedMps;
	std::array<double, kFbwResponseDelayFrames> delayed_commands{};
	std::size_t delay_index = 0;
};

struct AltitudeCaptureMetrics
{
	double maximum_error_m = 0.0;
	double settled_vertical_speed_mps = 0.0;
	int error_crossings = 0;
	int saturated_frames = 0;
};

void step_vertical_plant(VerticalPlant& plant, double pitch_command)
{
	const double delayed_command =
		plant.delayed_commands[plant.delay_index];
	plant.delayed_commands[plant.delay_index] = pitch_command;
	plant.delay_index =
		(plant.delay_index + 1) % plant.delayed_commands.size();
	const double acceleration_mps2 =
		delayed_command * kMeasuredGPerPitchCommand * kGravityMps2;
	plant.vertical_speed_mps += acceleration_mps2 * kDcsFrameDtS;
	plant.altitude_m += plant.vertical_speed_mps * kDcsFrameDtS;
}

void update_capture_metrics(
	AltitudeCaptureMetrics& metrics,
	double previous_error_m,
	double error_m,
	double pitch_command)
{
	metrics.maximum_error_m =
		std::max(metrics.maximum_error_m, std::abs(error_m));
	if (previous_error_m * error_m < 0.0)
		++metrics.error_crossings;
	if (std::abs(pitch_command) >= kPitchCommandSaturationThreshold)
		++metrics.saturated_frames;
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
}

AltitudeCaptureMetrics run_altitude_capture()
{
	AutomaticFlightControl control = make_control();
	AutomaticFlightControlObservation observation = nominal_observation();
	observation.dt_s = kDcsFrameDtS;
	observation.vertical_speed_mps =
		kAltitudeCaptureInitialVerticalSpeedMps;
	prime_and_engage(control, observation);
	send(control, CommandId::SelectAutopilotAltitudeHold);
	VerticalPlant plant;
	AltitudeCaptureMetrics metrics;
	double previous_error_m = 0.0;
	const int frame_count =
		static_cast<int>(kAltitudeCaptureDurationS / kDcsFrameDtS);
	for (int frame = 0; frame < frame_count; ++frame)
	{
		observation.altitude_m = plant.altitude_m;
		observation.vertical_speed_mps = plant.vertical_speed_mps;
		const double command =
			control.step(observation).pitch_normalized;
		step_vertical_plant(plant, command);
		const double error_m = kNominalAltitudeM - plant.altitude_m;
		update_capture_metrics(
			metrics, previous_error_m, error_m, command);
		previous_error_m = error_m;
	}
	metrics.settled_vertical_speed_mps = plant.vertical_speed_mps;
	return metrics;
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
	TEST_EXPECT(context, demand.pitch_normalized < 0.0);
	TEST_EXPECT(context, demand.roll_normalized < 0.0);
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
		control.step(observation).pitch_normalized,
		-0.059,
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
	observation.vertical_speed_mps = 3.0;
	TEST_EXPECT_NEAR(
		context,
		control.step(observation).pitch_normalized,
		0.0804,
		kTolerance);
}

void test_altitude_capture_is_damped_with_measured_fbw_response(
	Tests::Context& context)
{
	const AltitudeCaptureMetrics metrics = run_altitude_capture();
	TEST_EXPECT(context, metrics.maximum_error_m <= kMaximumCaptureErrorM);
	TEST_EXPECT(
		context,
		std::abs(metrics.settled_vertical_speed_mps) <=
			kMaximumSettledVerticalSpeedMps);
	TEST_EXPECT(
		context,
		metrics.error_crossings <= kMaximumCaptureErrorCrossings);
	TEST_EXPECT(context, metrics.saturated_frames == 0);
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
		control.step(observation).roll_normalized,
		-0.049036,
		kTolerance);
}

void test_vertical_modes_and_adjustments(Tests::Context& context)
{
	AutomaticFlightControl control = make_control();
	AutomaticFlightControlObservation observation = nominal_observation();
	observation.vertical_speed_mps = 4.0;
	prime_and_engage(control, observation);
	send(control, CommandId::SelectAutopilotVerticalSpeedHold);
	TEST_EXPECT_NEAR(
		context,
		control.snapshot().target_vertical_speed_mps,
		4.0,
		kTolerance);
	send(control, CommandId::IncreaseAutopilotVerticalReference);
	TEST_EXPECT_NEAR(
		context,
		control.snapshot().target_vertical_speed_mps,
		5.0,
		kTolerance);
	observation.vertical_speed_mps = 3.0;
	TEST_EXPECT(context, control.step(observation).pitch_normalized > 0.0);

	send(control, CommandId::SelectAutopilotAltitudeHold);
	send(control, CommandId::IncreaseAutopilotVerticalReference);
	TEST_EXPECT_NEAR(
		context,
		control.snapshot().target_altitude_m,
		kNominalAltitudeM + 30.48,
		kTolerance);
	observation.altitude_m = kNominalAltitudeM - 100.0;
	TEST_EXPECT(context, control.step(observation).pitch_normalized > 0.0);
}

void test_heading_wrap_and_navigation_placeholder(Tests::Context& context)
{
	AutomaticFlightControl control = make_control();
	AutomaticFlightControlObservation observation = nominal_observation();
	observation.heading_rad = Common::rad(359.5);
	prime_and_engage(control, observation);
	send(control, CommandId::IncreaseAutopilotLateralReference);
	TEST_EXPECT_NEAR(
		context,
		control.snapshot().target_heading_rad,
		Common::rad(0.5),
		kTolerance);
	send(control, CommandId::SelectAutopilotNavigationTrack);
	TEST_EXPECT(
		context,
		control.snapshot().lateral_mode ==
			AutomaticFlightControlLateralMode::NavigationTrack);
	observation.roll_rad = Common::rad(20.0);
	TEST_EXPECT_NEAR(
		context,
		control.step(observation).roll_normalized,
		0.0,
		kTolerance);
}

void test_bypass_freeze_and_recapture(Tests::Context& context)
{
	AutomaticFlightControl control = make_control();
	AutomaticFlightControlObservation observation = nominal_observation();
	prime_and_engage(control, observation);
	send(control, CommandId::SetAutopilotBypass);
	observation.pitch_rad += Common::rad(2.0);
	observation.heading_rad += Common::rad(2.0);
	const auto& bypass_demand = control.step(observation);
	TEST_EXPECT(context, control.snapshot().bypass_active);
	TEST_EXPECT_NEAR(
		context, bypass_demand.pitch_normalized, 0.0, kTolerance);
	TEST_EXPECT_NEAR(
		context, bypass_demand.roll_normalized, 0.0, kTolerance);
	send(control, CommandId::SetAutopilotBypass, 0.0);
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

void test_bypass_ignores_sub_threshold_change(Tests::Context& context)
{
	AutomaticFlightControl control = make_control();
	AutomaticFlightControlObservation observation = nominal_observation();
	prime_and_engage(control, observation);
	const double target_pitch = control.snapshot().target_pitch_rad;
	const double target_heading = control.snapshot().target_heading_rad;
	send(control, CommandId::SetAutopilotBypass);
	observation.pitch_rad += Common::rad(0.5);
	observation.heading_rad += Common::rad(0.5);
	(void)control.step(observation);
	send(control, CommandId::SetAutopilotBypass, 0.0);
	TEST_EXPECT_NEAR(
		context, control.snapshot().target_pitch_rad, target_pitch, kTolerance);
	TEST_EXPECT_NEAR(
		context, control.snapshot().target_heading_rad, target_heading, kTolerance);
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
	TEST_EXPECT(context, boundary.snapshot().auto_throttle_engaged);

	observation.mach = 0.951;
	AutomaticFlightControl fast = make_control();
	(void)fast.step(observation);
	send(fast, CommandId::EngageAutoThrottle);
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
	TEST_EXPECT_NEAR(
		context,
		control.snapshot().target_speed_mps,
		knots(550.0),
		kTolerance);
	send(control, CommandId::IncreaseAutopilotSpeed);
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
	TEST_EXPECT_NEAR(
		context,
		minimum.snapshot().target_speed_mps,
		knots(200.0),
		kTolerance);
	send(minimum, CommandId::DecreaseAutopilotSpeed);
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
	observation.indicated_airspeed_mps -= 1.0;
	TEST_EXPECT_NEAR(
		context,
		control.step(observation).throttle_normalized,
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
}

void run_automatic_flight_control_tests(Tests::Context& context)
{
	test_autopilot_engage_boundaries(context);
	test_attitude_and_ground_engage_gates(context);
	test_attitude_boundary_is_inclusive(context);
	test_default_capture_and_controller_direction(context);
	test_pitch_controller_characterization(context);
	test_vertical_speed_controller_characterization(context);
	test_altitude_capture_is_damped_with_measured_fbw_response(context);
	test_heading_controller_characterization(context);
	test_vertical_modes_and_adjustments(context);
	test_heading_wrap_and_navigation_placeholder(context);
	test_bypass_freeze_and_recapture(context);
	test_bypass_ignores_sub_threshold_change(context);
	test_disconnect_guards(context);
	test_auto_throttle_engage_gates(context);
	test_mach_guard_uses_elapsed_time(context);
	test_auto_throttle_target_limits(context);
	test_auto_throttle_controller_characterization(context);
	test_commanded_disconnect_resets_both_channels(context);
}
