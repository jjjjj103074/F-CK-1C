#include "TestHarness.h"
#include "AutomaticFlightControlTestSupport.h"

namespace
{
using namespace AutomaticFlightControlTestSupport;

constexpr double kMonitorPersistenceFraction = 0.6;
constexpr double kSustainedFailureStepS = 3.0;
constexpr double kVerticalTrackingFailureMps = 13.0;

void test_sustained_failure_changes_mode_on_next_tick(
	Tests::Context& context)
{
	AutomaticFlightControl control = make_control();
	const AutomaticFlightControlObservation observation = nominal_observation();
	prime_and_engage(control, observation);
	send(control, CommandId::SelectAutopilotVerticalSpeedHold);
	(void)step(control, observation);
	Core::Systems::AutopilotModeMonitorObservation monitor;
	monitor.dt_s = kSustainedFailureStepS;
	monitor.vertical_active = true;
	monitor.vertical_type =
		Core::Systems::VerticalGuidanceReferenceType::VerticalSpeed;
	monitor.vertical_tracking_error = kVerticalTrackingFailureMps;
	control.observe_control_result(monitor);
	(void)step(control, observation);
	control.observe_control_result(monitor);
	TEST_EXPECT(
		context,
		control.snapshot().vertical_mode ==
			AutomaticFlightControlVerticalMode::VerticalSpeedHold);
	(void)step(control, observation);
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

void test_mode_change_resets_saturation_persistence(
	Tests::Context& context)
{
	const auto config =
		Core::Systems::fck1c_automatic_flight_control_config();
	AutomaticFlightControl control(config, false);
	const AutomaticFlightControlObservation observation = nominal_observation();
	prime_and_engage(control, observation);
	Core::Systems::AutopilotModeMonitorObservation monitor;
	monitor.dt_s = config.actuator_saturation_persistence_s *
		kMonitorPersistenceFraction;
	monitor.vertical_active = true;
	monitor.lateral_active = true;
	monitor.vertical_type =
		Core::Systems::VerticalGuidanceReferenceType::PitchAttitude;
	monitor.actuator_saturated = true;
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
}

void run_automatic_flight_control_lifecycle_tests(Tests::Context& context)
{
	test_sustained_failure_changes_mode_on_next_tick(context);
	test_mode_change_resets_saturation_persistence(context);
}
