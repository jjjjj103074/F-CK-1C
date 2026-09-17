#include "TestHarness.h"
#include "AutomaticFlightControlTestSupport.h"

#include "Common/Units.h"

#include <limits>

namespace
{
using namespace AutomaticFlightControlTestSupport;

Core::Systems::AutomaticFlightControlConfig test_config()
{
	auto result = Core::Systems::fck1c_automatic_flight_control_config();
	result.pitch_tracking_error_limit_rad = 0.1;
	result.vertical_speed_tracking_error_limit_ft_s = 2.0;
	result.bank_tracking_error_limit_rad = 0.2;
	result.tracking_failure_persistence_s = 0.1;
	result.actuator_saturation_persistence_s = 0.08;
	return result;
}

Core::Systems::FlightControlCommandMonitorInput vertical_failure()
{
	Core::Systems::FlightControlCommandMonitorInput observation;
	observation.dt_s = 0.04;
	observation.vertical_active = true;
	observation.vertical_type =
		Core::Systems::VerticalGuidanceReferenceType::VerticalSpeed;
	observation.vertical_speed_tracking_error_ft_s = Common::feet(3.0);
	return observation;
}

void engage_altitude(
	AutomaticFlightControl& control,
	const AutomaticFlightControlObservation& observation)
{
	(void)step(control, observation);
	send(control, CommandId::SelectAutopilotAltitudeHold);
	prime_and_engage(control, observation);
}

void apply_monitor(
	AutomaticFlightControl& control,
	const Core::Systems::FlightControlCommandMonitorInput& monitor,
	const AutomaticFlightControlObservation& observation)
{
	control.observe_control_result(monitor);
	(void)step(control, observation);
}

void test_transient_failure_keeps_mode(Tests::Context& context)
{
	AutomaticFlightControl control(test_config(), false);
	const auto observation = nominal_observation();
	engage_altitude(control, observation);
	apply_monitor(control, vertical_failure(), observation);
	TEST_EXPECT(context, control.snapshot().vertical_mode ==
		AutomaticFlightControlVerticalMode::AltitudeHold);
	TEST_EXPECT(context, control.snapshot().degradation_reason ==
		Core::FlightControlDegradationReason::None);
}

void test_sustained_vertical_failure_releases_only_vertical(
	Tests::Context& context)
{
	AutomaticFlightControl control(test_config(), false);
	const auto observation = nominal_observation();
	engage_altitude(control, observation);
	const auto monitor = vertical_failure();
	apply_monitor(control, monitor, observation);
	apply_monitor(control, monitor, observation);
	apply_monitor(control, monitor, observation);
	TEST_EXPECT(context, control.snapshot().vertical_mode ==
		AutomaticFlightControlVerticalMode::Off);
	TEST_EXPECT(context, control.snapshot().lateral_mode ==
		AutomaticFlightControlLateralMode::RollAttitudeHold);
	TEST_EXPECT(context, control.snapshot().degradation_reason ==
		Core::FlightControlDegradationReason::SustainedVerticalTrackingFailure);
}

void test_recovery_clears_persistence_timer(Tests::Context& context)
{
	AutomaticFlightControl control(test_config(), false);
	const auto observation = nominal_observation();
	engage_altitude(control, observation);
	auto monitor = vertical_failure();
	apply_monitor(control, monitor, observation);
	apply_monitor(control, monitor, observation);
	monitor.vertical_speed_tracking_error_ft_s = 0.0;
	apply_monitor(control, monitor, observation);
	monitor = vertical_failure();
	apply_monitor(control, monitor, observation);
	apply_monitor(control, monitor, observation);
	TEST_EXPECT(context, control.snapshot().vertical_mode ==
		AutomaticFlightControlVerticalMode::AltitudeHold);
}

void test_sustained_saturation_releases_active_axes(
	Tests::Context& context)
{
	AutomaticFlightControl control(test_config(), false);
	const auto observation = nominal_observation();
	engage_altitude(control, observation);
	auto monitor = vertical_failure();
	monitor.vertical_speed_tracking_error_ft_s = 0.0;
	monitor.lateral_active = true;
	monitor.control_path_saturated = true;
	apply_monitor(control, monitor, observation);
	apply_monitor(control, monitor, observation);
	TEST_EXPECT(context, control.snapshot().vertical_mode ==
		AutomaticFlightControlVerticalMode::Off);
	TEST_EXPECT(context, control.snapshot().lateral_mode ==
		AutomaticFlightControlLateralMode::Off);
	TEST_EXPECT(context, control.snapshot().degradation_reason ==
		Core::FlightControlDegradationReason::SustainedActuatorSaturation);
}

void test_inactive_saturation_does_not_cross_engagement_boundary(
	Tests::Context& context)
{
	AutomaticFlightControl control(test_config(), false);
	const auto observation = nominal_observation();
	auto monitor = vertical_failure();
	monitor.vertical_active = false;
	monitor.vertical_speed_tracking_error_ft_s = 0.0;
	monitor.control_path_saturated = true;
	apply_monitor(control, monitor, observation);
	apply_monitor(control, monitor, observation);
	engage_altitude(control, observation);
	apply_monitor(control, vertical_failure(), observation);
	TEST_EXPECT(context, control.snapshot().vertical_mode ==
		AutomaticFlightControlVerticalMode::AltitudeHold);
}

void test_transient_constraint_reason_is_exposed(Tests::Context& context)
{
	AutomaticFlightControl control(test_config(), false);
	const auto observation = nominal_observation();
	engage_altitude(control, observation);
	auto monitor = vertical_failure();
	monitor.vertical_speed_tracking_error_ft_s = 0.0;
	monitor.constraint.reason =
		Core::Systems::ConstraintReason::VerticalReferenceUnmaintainable;
	monitor.constraint.vertical_constrained = true;
	apply_monitor(control, monitor, observation);
	TEST_EXPECT(context, control.snapshot().constraint_reason ==
		Core::FlightControlConstraintReason::VerticalReferenceUnmaintainable);
	TEST_EXPECT(context, control.snapshot().vertical_mode ==
		AutomaticFlightControlVerticalMode::AltitudeHold);
}

void test_invalid_input_requests_typed_disconnect(Tests::Context& context)
{
	AutomaticFlightControl control(test_config(), false);
	const auto observation = nominal_observation();
	engage_altitude(control, observation);
	auto monitor = vertical_failure();
	monitor.vertical_speed_tracking_error_ft_s =
		std::numeric_limits<double>::quiet_NaN();
	apply_monitor(control, monitor, observation);
	TEST_EXPECT(context, !control.snapshot().master_engaged);
	TEST_EXPECT(context, control.snapshot().disconnect_reason ==
		Core::FlightControlDisconnectReason::InvalidInput);
}
}

void run_autopilot_mode_monitor_tests(Tests::Context& context)
{
	test_transient_failure_keeps_mode(context);
	test_sustained_vertical_failure_releases_only_vertical(context);
	test_recovery_clears_persistence_timer(context);
	test_sustained_saturation_releases_active_axes(context);
	test_inactive_saturation_does_not_cross_engagement_boundary(context);
	test_transient_constraint_reason_is_exposed(context);
	test_invalid_input_requests_typed_disconnect(context);
}
