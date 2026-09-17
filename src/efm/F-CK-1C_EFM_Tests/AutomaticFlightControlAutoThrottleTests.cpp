#include "TestHarness.h"
#include "AutomaticFlightControlTestSupport.h"

#include <iterator>

namespace
{
using namespace AutomaticFlightControlTestSupport;

void test_auto_throttle_engage_gates(Tests::Context& context)
{
	AutomaticFlightControlObservation observation = nominal_observation();
	observation.mach = 0.95;
	AutomaticFlightControl boundary = make_control();
	(void)step(boundary, observation);
	send(boundary, CommandId::EngageAutoThrottle);
	(void)step(boundary, observation);
	TEST_EXPECT(context, boundary.snapshot().auto_throttle_engaged);

	observation.mach = 0.951;
	AutomaticFlightControl fast = make_control();
	(void)step(fast, observation);
	send(fast, CommandId::EngageAutoThrottle);
	(void)step(fast, observation);
	TEST_EXPECT(context, !fast.snapshot().auto_throttle_engaged);
	TEST_EXPECT(context,
		fast.snapshot().auto_throttle_engage_rejection_reason ==
			AutomaticFlightControlReason::MachLimit);

	observation = nominal_observation();
	observation.weight_on_wheels = true;
	AutomaticFlightControl ground = make_control();
	(void)step(ground, observation);
	send(ground, CommandId::EngageAutoThrottle);
	(void)step(ground, observation);
	TEST_EXPECT(context, !ground.snapshot().auto_throttle_engaged);
	TEST_EXPECT(context,
		ground.snapshot().auto_throttle_engage_rejection_reason ==
			AutomaticFlightControlReason::WeightOnWheels);
}

double run_mach_guard(const double* steps, std::size_t count)
{
	AutomaticFlightControl control = make_control();
	AutomaticFlightControlObservation observation = nominal_observation();
	(void)step(control, observation);
	send(control, CommandId::EngageAutoThrottle);
	(void)step(control, observation);
	observation.mach = 0.96;
	for (std::size_t index = 0; index < count; ++index)
	{
		observation.dt_s = steps[index];
		(void)step(control, observation);
	}
	return control.snapshot().throttle_command_normalized;
}

void test_mach_guard_uses_elapsed_time(Tests::Context& context)
{
	const double fixed[] = { 0.02 };
	const double irregular[] = { 0.003, 0.007, 0.01 };
	TEST_EXPECT_NEAR(context,
		run_mach_guard(fixed, std::size(fixed)),
		run_mach_guard(irregular, std::size(irregular)), kTolerance);
}

void test_auto_throttle_target_limits(Tests::Context& context)
{
	AutomaticFlightControl control = make_control();
	AutomaticFlightControlObservation observation = nominal_observation();
	observation.indicated_airspeed_mps = knots(548.0);
	(void)step(control, observation);
	send(control, CommandId::EngageAutoThrottle);
	send(control, CommandId::IncreaseAutopilotSpeed);
	(void)step(control, observation);
	TEST_EXPECT_NEAR(context, control.snapshot().target_speed_mps,
		knots(550.0), kTolerance);
	send(control, CommandId::IncreaseAutopilotSpeed);
	(void)step(control, observation);
	TEST_EXPECT_NEAR(context, control.snapshot().target_speed_mps,
		knots(550.0), kTolerance);

	AutomaticFlightControl minimum = make_control();
	observation.indicated_airspeed_mps = knots(202.0);
	(void)step(minimum, observation);
	send(minimum, CommandId::EngageAutoThrottle);
	send(minimum, CommandId::DecreaseAutopilotSpeed);
	(void)step(minimum, observation);
	TEST_EXPECT_NEAR(context, minimum.snapshot().target_speed_mps,
		knots(200.0), kTolerance);
	send(minimum, CommandId::DecreaseAutopilotSpeed);
	(void)step(minimum, observation);
	TEST_EXPECT_NEAR(context, minimum.snapshot().target_speed_mps,
		knots(200.0), kTolerance);
}

void test_auto_throttle_controller_characterization(
	Tests::Context& context)
{
	AutomaticFlightControl control = make_control();
	AutomaticFlightControlObservation observation = nominal_observation();
	(void)step(control, observation);
	send(control, CommandId::EngageAutoThrottle);
	(void)step(control, observation);
	observation.indicated_airspeed_mps -= 1.0;
	TEST_EXPECT_NEAR(context,
		step(control, observation).experimental_throttle_normalized,
		0.51506, kTolerance);
}

void test_developer_availability_is_explicit(Tests::Context& context)
{
	const auto config =
		Core::Systems::fck1c_automatic_flight_control_config();
	AutomaticFlightControl disabled(config, false, false);
	AutomaticFlightControl enabled(config, false, true);
	TEST_EXPECT(context,
		!disabled.snapshot().experimental_auto_throttle_available);
	TEST_EXPECT(context,
		enabled.snapshot().experimental_auto_throttle_available);
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
	(void)step(control, observation);
	TEST_EXPECT(context, !control.snapshot().master_engaged);
	TEST_EXPECT(context, !control.snapshot().auto_throttle_engaged);
	TEST_EXPECT(context,
		control.snapshot().autopilot_disengage_reason ==
			AutomaticFlightControlReason::Commanded);
	TEST_EXPECT(context,
		control.snapshot().auto_throttle_disengage_reason ==
			AutomaticFlightControlReason::Commanded);
}
}

void run_automatic_flight_control_auto_throttle_tests(
	Tests::Context& context)
{
	test_auto_throttle_engage_gates(context);
	test_mach_guard_uses_elapsed_time(context);
	test_auto_throttle_target_limits(context);
	test_auto_throttle_controller_characterization(context);
	test_developer_availability_is_explicit(context);
	test_commanded_disconnect_resets_both_channels(context);
}
