#include "TestHarness.h"

#include "Common/Units.h"
#include "Core/Systems/FlightControlComputer/ControlLaws/ControlLaws.h"
#include "DcsBridge/Internal/DcsKinematicsAdapter.h"

namespace
{
constexpr double kTolerance = 1e-9;
constexpr double kControlDtS = 1.0 / 64.0;

void test_dcs_body_axes_are_named_without_changing_handedness(
	Tests::Context& context)
{
	const auto converted =
		DcsBridge::Internal::adapt_dcs_body_angular_kinematics(
			{ 1.0, 2.0, 3.0 },
			{ 4.0, 5.0, 6.0 });
	TEST_EXPECT_NEAR(context, converted.roll_acceleration_rad_s2, 1.0, kTolerance);
	TEST_EXPECT_NEAR(context, converted.pitch_acceleration_rad_s2, 3.0, kTolerance);
	TEST_EXPECT_NEAR(context, converted.yaw_acceleration_rad_s2, 2.0, kTolerance);
	TEST_EXPECT_NEAR(context, converted.roll_rate_rad_s, 4.0, kTolerance);
	TEST_EXPECT_NEAR(context, converted.pitch_rate_rad_s, 6.0, kTolerance);
	TEST_EXPECT_NEAR(context, converted.yaw_rate_rad_s, 5.0, kTolerance);
}

void test_positive_dcs_yaw_rate_produces_opposing_rudder_damping(
	Tests::Context& context)
{
	constexpr double kPositiveDcsYawRateRadS = 0.2;
	const auto converted =
		DcsBridge::Internal::adapt_dcs_body_angular_kinematics(
			{}, { 0.0, kPositiveDcsYawRateRadS, 0.0 });
	Systems::ModeAndGainScheduling scheduling(
		Systems::make_fck1c_mode_and_gain_scheduling_config());
	Systems::FlightControlLawsInput input;
	input.flight.dt_s = kControlDtS;
	input.flight.dynamic_pressure_pa = 5000.0;
	input.flight.normal_acceleration_g = 1.0;
	input.flight.yaw_rate_rad_s = converted.yaw_rate_rad_s;
	input.configuration = scheduling.update({
		kControlDtS, 5000.0, 0.7, true, false });
	Systems::FlightControlLaws laws({});
	const auto output = laws.update(input);
	TEST_EXPECT(
		context, output.normal_surface_demand.rudder_demand_rad < 0.0);
}
}

void run_dcs_kinematics_adapter_tests(Tests::Context& context)
{
	test_dcs_body_axes_are_named_without_changing_handedness(context);
	test_positive_dcs_yaw_rate_produces_opposing_rudder_damping(context);
}
