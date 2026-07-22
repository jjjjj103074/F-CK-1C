#include "TestHarness.h"

#include "DcsBridge/Internal/CarrierBridge.h"

namespace
{
constexpr double kTolerance = 1e-6;
constexpr double kReferenceThrustN = 25000.0;
constexpr double kLaunchThresholdThrottle = 0.99;
constexpr double kFullThrottle = 1.0;
constexpr double kExpectedLaunchDelayS = 2.0;
constexpr double kExpectedAddedVelocityMps = 80.0;
constexpr float kArmedPhase = 1.0F;
constexpr float kStartedPhase = 2.0F;
constexpr float kFinishedPhase = 3.0F;
constexpr float kUnknownPhase = 1.5F;

ed_fm_simulation_event carrier_phase_event(float phase)
{
	ed_fm_simulation_event event = {};
	event.event_type = ED_FM_EVENT_CARRIER_CATAPULT;
	event.event_params[0] = phase;
	return event;
}

void test_carrier_launch_pop(Tests::Context& context)
{
	DcsBridge::Internal::CarrierBridge bridge({ kReferenceThrustN });
	ed_fm_simulation_event output = {};
	TEST_EXPECT(context, !bridge.pop_event({ kFullThrottle }, output));
	const ed_fm_simulation_event armed = carrier_phase_event(kArmedPhase);
	TEST_EXPECT(context, !bridge.push_event(armed));
	TEST_EXPECT(context, !bridge.pop_event({ kLaunchThresholdThrottle }, output));
	TEST_EXPECT(context, bridge.pop_event({ kFullThrottle }, output));
	TEST_EXPECT(context, output.event_type == ED_FM_EVENT_CARRIER_CATAPULT);
	TEST_EXPECT_NEAR(context, output.event_params[0], kArmedPhase, kTolerance);
	TEST_EXPECT_NEAR(context, output.event_params[1], kExpectedLaunchDelayS, kTolerance);
	TEST_EXPECT_NEAR(
		context, output.event_params[2], kExpectedAddedVelocityMps, kTolerance);
	TEST_EXPECT_NEAR(context, output.event_params[3], kReferenceThrustN, kTolerance);
	TEST_EXPECT(context, !bridge.pop_event({ kFullThrottle }, output));
}

void test_carrier_push_phases_and_reset(Tests::Context& context)
{
	DcsBridge::Internal::CarrierBridge bridge({ kReferenceThrustN });
	ed_fm_simulation_event output = {};
	TEST_EXPECT(context, !bridge.push_event(carrier_phase_event(kArmedPhase)));
	TEST_EXPECT(context, !bridge.push_event(carrier_phase_event(kStartedPhase)));
	TEST_EXPECT(context, !bridge.pop_event({ kFullThrottle }, output));
	TEST_EXPECT(context, !bridge.push_event(carrier_phase_event(kFinishedPhase)));
	TEST_EXPECT(context, !bridge.pop_event({ kFullThrottle }, output));
	TEST_EXPECT(context, !bridge.push_event(carrier_phase_event(kArmedPhase)));
	bridge.reset();
	TEST_EXPECT(context, !bridge.pop_event({ kFullThrottle }, output));
}

void test_non_carrier_event_is_ignored(Tests::Context& context)
{
	DcsBridge::Internal::CarrierBridge bridge({ kReferenceThrustN });
	ed_fm_simulation_event input = {};
	ed_fm_simulation_event output = {};
	TEST_EXPECT(context, !bridge.push_event(input));
	TEST_EXPECT(context, !bridge.pop_event({ kFullThrottle }, output));
	TEST_EXPECT(context, !bridge.push_event(carrier_phase_event(kUnknownPhase)));
	TEST_EXPECT(context, !bridge.pop_event({ kFullThrottle }, output));
}
}

void run_carrier_bridge_tests(Tests::Context& context)
{
	test_carrier_launch_pop(context);
	test_carrier_push_phases_and_reset(context);
	test_non_carrier_event_is_ignored(context);
}
