#include "TestHarness.h"

#include "DcsBridge/ParamExport.h"

namespace
{
constexpr double kTolerance = 1e-9;

DcsBridge::ParamExportState make_state()
{
	DcsBridge::ParamExportState state = {};
	state.suspension_feedback_available = true;
	state.any_weight_on_wheels = true;
	state.gear_pos = 1.0;
	state.nose_wheel_steering = -0.25;
	state.wheel_spin[0] = 1.0;
	state.wheel_spin[1] = 2.0;
	state.wheel_spin[2] = 3.0;
	state.wheel_brake_left = 0.6;
	state.wheel_brake_right = 0.7;
	state.pitch_input = 0.2;
	state.roll_input = -0.3;
	state.yaw_input = 0.4;
	state.left_engine_switch = true;
	state.left_throttle_input = 0.8;
	state.left_throttle_output = 0.9;
	state.left_engine_power_readout = 0.5;
	state.left_thrust_force = 12000.0;
	state.atmosphere_temperature = 288.0;
	state.internal_fuel = 900.0;
	state.total_fuel = 1100.0;
	return state;
}

void test_wheel_and_control_params(Tests::Context& context)
{
	using namespace DcsIds::Params;
	const DcsBridge::ParamExportState state = make_state();
	TEST_EXPECT_NEAR(context, DcsBridge::get_param(NoseWheelYaw, state), -0.25, kTolerance);
	TEST_EXPECT_NEAR(context, DcsBridge::get_param(RightWheelSpin, state), 3.0, kTolerance);
	TEST_EXPECT_NEAR(context, DcsBridge::get_param(LeftBrakeMoment, state), 0.6, kTolerance);
	TEST_EXPECT_NEAR(context, DcsBridge::get_param(StickPitch, state), 0.2, kTolerance);
	TEST_EXPECT_NEAR(context, DcsBridge::get_param(StickRoll, state), -0.3, kTolerance);
	TEST_EXPECT_NEAR(context, DcsBridge::get_param(RudderPedals, state), -0.4, kTolerance);
	TEST_EXPECT_NEAR(context, DcsBridge::get_param(ThrottleLeft, state), 0.8, kTolerance);
}

void test_service_and_engine_params(Tests::Context& context)
{
	using namespace DcsIds::Params;
	const DcsBridge::ParamExportState state = make_state();
	TEST_EXPECT_NEAR(context, DcsBridge::get_param(InternalFuel, state), 900.0, kTolerance);
	TEST_EXPECT_NEAR(context, DcsBridge::get_param(TotalFuel, state), 1100.0, kTolerance);
	TEST_EXPECT_NEAR(context, DcsBridge::get_param(ApuRelatedRpm, state), 1.0, kTolerance);
	TEST_EXPECT_NEAR(context, DcsBridge::get_param(LeftEngineCoreRpm, state), 9929.25, kTolerance);
	TEST_EXPECT_NEAR(context, DcsBridge::get_param(LeftEngineRpm, state), 5545.125, kTolerance);
	TEST_EXPECT_NEAR(context, DcsBridge::get_param(LeftEngineCombustion, state), 1.0, kTolerance);
	TEST_EXPECT_NEAR(context, DcsBridge::get_param(LeftEngineThrust, state), 12000.0, kTolerance);
	TEST_EXPECT_NEAR(context, DcsBridge::get_param(999999, state), 0.0, kTolerance);
}
}

void run_param_export_tests(Tests::Context& context)
{
	test_wheel_and_control_params(context);
	test_service_and_engine_params(context);
}
