#include "TestHarness.h"

#include "DcsBridge/Internal/DcsKinematicsAdapter.h"

namespace
{
constexpr double kTolerance = 1e-9;

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

}

void run_dcs_kinematics_adapter_tests(Tests::Context& context)
{
	test_dcs_body_axes_are_named_without_changing_handedness(context);
}
