#include "TestHarness.h"

#include "Core/Systems/FlightControlComputer/Autopilot/AutopilotModeMonitor.h"

#include <limits>

namespace
{
Core::Systems::AutopilotModeMonitorConfig test_config()
{
	return { 0.1, 2.0, 0.2, 0.1, 0.08 };
}

Core::Systems::AutopilotModeMonitorObservation vertical_failure()
{
	Core::Systems::AutopilotModeMonitorObservation observation;
	observation.dt_s = 0.04;
	observation.vertical_active = true;
	observation.vertical_type =
		Core::Systems::VerticalGuidanceReferenceType::VerticalSpeed;
	observation.vertical_tracking_error = 3.0;
	return observation;
}

void test_transient_failure_keeps_mode(Tests::Context& context)
{
	Core::Systems::AutopilotModeMonitor monitor(test_config());
	const auto result = monitor.update(vertical_failure());
	TEST_EXPECT(context, !result.release_vertical);
	TEST_EXPECT(
		context,
		result.degradation_reason == Core::Systems::DegradationReason::None);
}

void test_sustained_vertical_failure_releases_only_vertical(
	Tests::Context& context)
{
	Core::Systems::AutopilotModeMonitor monitor(test_config());
	const auto observation = vertical_failure();
	(void)monitor.update(observation);
	(void)monitor.update(observation);
	const auto result = monitor.update(observation);
	TEST_EXPECT(context, result.release_vertical);
	TEST_EXPECT(context, !result.release_lateral);
	TEST_EXPECT(
		context,
		result.degradation_reason ==
			Core::Systems::DegradationReason::SustainedVerticalTrackingFailure);
}

void test_recovery_clears_persistence_timer(Tests::Context& context)
{
	Core::Systems::AutopilotModeMonitor monitor(test_config());
	auto observation = vertical_failure();
	(void)monitor.update(observation);
	(void)monitor.update(observation);
	observation.vertical_tracking_error = 0.0;
	(void)monitor.update(observation);
	observation.vertical_tracking_error = 3.0;
	(void)monitor.update(observation);
	const auto result = monitor.update(observation);
	TEST_EXPECT(context, !result.release_vertical);
}

void test_sustained_saturation_releases_active_axes(
	Tests::Context& context)
{
	Core::Systems::AutopilotModeMonitor monitor(test_config());
	auto observation = vertical_failure();
	observation.vertical_tracking_error = 0.0;
	observation.lateral_active = true;
	observation.actuator_saturated = true;
	(void)monitor.update(observation);
	const auto result = monitor.update(observation);
	TEST_EXPECT(context, result.release_vertical);
	TEST_EXPECT(context, result.release_lateral);
	TEST_EXPECT(
		context,
		result.degradation_reason ==
			Core::Systems::DegradationReason::SustainedActuatorSaturation);
}

void test_invalid_input_requests_typed_disconnect(Tests::Context& context)
{
	Core::Systems::AutopilotModeMonitor monitor(test_config());
	auto observation = vertical_failure();
	observation.vertical_tracking_error =
		std::numeric_limits<double>::quiet_NaN();
	const auto result = monitor.update(observation);
	TEST_EXPECT(
		context,
		result.disconnect_reason ==
			Core::Systems::DisconnectReason::InvalidInput);
}
}

void run_autopilot_mode_monitor_tests(Tests::Context& context)
{
	test_transient_failure_keeps_mode(context);
	test_sustained_vertical_failure_releases_only_vertical(context);
	test_recovery_clears_persistence_timer(context);
	test_sustained_saturation_releases_active_axes(context);
	test_invalid_input_requests_typed_disconnect(context);
}
