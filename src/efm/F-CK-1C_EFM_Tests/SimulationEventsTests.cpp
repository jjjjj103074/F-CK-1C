#include "TestHarness.h"

#include "DcsBridge/SimulationEvents.h"

namespace
{
void test_carrier_launch_pop(Tests::Context& context)
{
	DcsBridge::CarrierLaunchState state;
	ed_fm_simulation_event event = {};
	TEST_EXPECT(context, !DcsBridge::pop_carrier_launch_event(
		state, event, { 1.0, 25000.0 }));
	event.event_type = ED_FM_EVENT_CARRIER_CATAPULT;
	event.event_params[0] = 1.0;
	DcsBridge::push_carrier_launch_event(state, event);
	TEST_EXPECT(context, state.phase == DcsBridge::CarrierLaunchPhase::Armed);
	TEST_EXPECT(context, !DcsBridge::pop_carrier_launch_event(
		state, event, { 0.99, 25000.0 }));
	TEST_EXPECT(context, DcsBridge::pop_carrier_launch_event(
		state, event, { 1.0, 25000.0 }));
	TEST_EXPECT(context, state.phase == DcsBridge::CarrierLaunchPhase::Issued);
	TEST_EXPECT_NEAR(context, event.event_params[1], 2.0, 1e-6);
	TEST_EXPECT_NEAR(context, event.event_params[2], 80.0, 1e-6);
	TEST_EXPECT_NEAR(context, event.event_params[3], 25000.0, 1e-6);
}

void test_carrier_launch_push_phases(Tests::Context& context)
{
	DcsBridge::CarrierLaunchState state;
	ed_fm_simulation_event event = {};
	event.event_type = ED_FM_EVENT_CARRIER_CATAPULT;
	event.event_params[0] = 2.0;
	DcsBridge::push_carrier_launch_event(state, event);
	TEST_EXPECT(context, state.phase == DcsBridge::CarrierLaunchPhase::Started);
	event.event_params[0] = 3.0;
	DcsBridge::push_carrier_launch_event(state, event);
	TEST_EXPECT(context, state.phase == DcsBridge::CarrierLaunchPhase::Idle);
	event.event_params[0] = 1.5;
	DcsBridge::push_carrier_launch_event(state, event);
	TEST_EXPECT(context, state.phase == DcsBridge::CarrierLaunchPhase::Idle);
}
}

void run_simulation_event_tests(Tests::Context& context)
{
	test_carrier_launch_pop(context);
	test_carrier_launch_push_phases(context);
}
