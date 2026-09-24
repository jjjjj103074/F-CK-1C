#include "TestHarness.h"
#include "AutomaticFlightControlTestSupport.h"

#include "Common/Units.h"

#include <cmath>

namespace
{
using namespace AutomaticFlightControlTestSupport;

constexpr double kMonitorPersistenceFraction = 0.6;
constexpr double kSustainedFailureStepS = 3.0;
constexpr double kVerticalTrackingFailureMps = 13.0;
constexpr double kLateralTrackingFailureRad = 13.0 / Common::kDegPerRad;
constexpr double kStickSteeringRollDeg = 47.0;

void test_sustained_failure_changes_mode_on_next_tick(
	Tests::Context& context)
{
	AutomaticFlightControl control = make_control();
	const AutomaticFlightControlObservation observation = nominal_observation();
	prime_and_engage(control, observation);
	send(control, CommandId::SelectAutopilotAltitudeHold);
	(void)step(control, observation);
	Core::Systems::FlightControlCommandMonitorInput monitor;
	monitor.dt_s = kSustainedFailureStepS;
	monitor.vertical_active = true;
	monitor.vertical_type =
		Core::Systems::VerticalGuidanceReferenceType::VerticalSpeed;
	monitor.vertical_speed_tracking_error_ft_s =
		Common::feet(kVerticalTrackingFailureMps);
	control.observe_control_result(monitor);
	(void)step(control, observation);
	control.observe_control_result(monitor);
	TEST_EXPECT(
		context,
		control.snapshot().vertical_mode ==
			AutomaticFlightControlVerticalMode::AltitudeHold);
	(void)step(control, observation);
	TEST_EXPECT(
		context,
		control.snapshot().vertical_mode ==
			AutomaticFlightControlVerticalMode::Off);
	TEST_EXPECT(
		context,
		control.snapshot().lateral_mode ==
			AutomaticFlightControlLateralMode::RollAttitudeHold);
	TEST_EXPECT(context, control.snapshot().vertical_degraded);
	send(control, CommandId::SelectAutopilotPitchAttitudeHold);
	(void)step(control, observation);
	TEST_EXPECT(context, !control.snapshot().vertical_degraded);
	TEST_EXPECT(
		context,
		control.snapshot().degradation_reason ==
			Core::FlightControlDegradationReason::None);
}

void test_mode_change_resets_saturation_persistence(
	Tests::Context& context)
{
	const auto config =
		Core::Systems::fck1c_automatic_flight_control_config();
	AutomaticFlightControl control(config, false);
	const AutomaticFlightControlObservation observation = nominal_observation();
	prime_and_engage(control, observation);
	Core::Systems::FlightControlCommandMonitorInput monitor;
	monitor.dt_s = config.actuator_saturation_persistence_s *
		kMonitorPersistenceFraction;
	monitor.vertical_active = true;
	monitor.lateral_active = true;
	monitor.vertical_type =
		Core::Systems::VerticalGuidanceReferenceType::PitchAttitude;
	monitor.control_path_saturated = true;
	control.observe_control_result(monitor);
	(void)step(control, observation);
	send(control, CommandId::SelectAutopilotAltitudeHold);
	(void)step(control, observation);
	control.observe_control_result(monitor);
	(void)step(control, observation);
	TEST_EXPECT(context, control.snapshot().master_engaged);
	TEST_EXPECT(
		context,
		control.snapshot().vertical_mode ==
			AutomaticFlightControlVerticalMode::AltitudeHold);
	TEST_EXPECT(context, !control.snapshot().vertical_degraded);
	TEST_EXPECT(context, !control.snapshot().lateral_degraded);
}

void test_lateral_reengagement_clears_lateral_degradation(
	Tests::Context& context)
{
	AutomaticFlightControl control = make_control();
	const auto observation = nominal_observation();
	prime_and_engage(control, observation);
	Core::Systems::FlightControlCommandMonitorInput monitor;
	monitor.dt_s = kSustainedFailureStepS;
	monitor.lateral_active = true;
	monitor.lateral_tracking_error_rad = kLateralTrackingFailureRad;
	control.observe_control_result(monitor);
	(void)step(control, observation);
	control.observe_control_result(monitor);
	(void)step(control, observation);
	TEST_EXPECT(context, control.snapshot().lateral_degraded);
	send(control, CommandId::SelectAutopilotRollAttitudeHold);
	(void)step(control, observation);
	TEST_EXPECT(context, !control.snapshot().lateral_degraded);
	TEST_EXPECT(
		context,
		control.snapshot().degradation_reason ==
			Core::FlightControlDegradationReason::None);
}

void test_roll_stick_steering_respects_guidance_bank_limit(
	Tests::Context& context)
{
	const auto config =
		Core::Systems::fck1c_automatic_flight_control_config();
	AutomaticFlightControl control(config, false);
	auto observation = nominal_observation();
	prime_and_engage(control, observation);
	observation.roll_rad = Common::rad(kStickSteeringRollDeg);
	observation.conditioned_roll_input_normalized = 1.0;
	(void)step(control, observation);
	observation.conditioned_roll_input_normalized = 0.0;
	(void)step(control, observation);
	TEST_EXPECT(
		context,
		std::abs(control.snapshot().bank_angle_reference_rad) <=
			Core::Systems::fck1c_flight_control_computer_config().
				values.mode_and_gain.guidance_bank_limit_rad + kTolerance);
}
}

void run_automatic_flight_control_lifecycle_tests(Tests::Context& context)
{
	test_sustained_failure_changes_mode_on_next_tick(context);
	test_mode_change_resets_saturation_persistence(context);
	test_lateral_reengagement_clears_lateral_degradation(context);
	test_roll_stick_steering_respects_guidance_bank_limit(context);
}
