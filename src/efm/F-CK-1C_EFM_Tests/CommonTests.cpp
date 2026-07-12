#include "TestHarness.h"

#include "Common/Actuator.h"
#include "Common/Clamp.h"
#include "Common/Table.h"
#include "Common/Units.h"

namespace
{
constexpr double kTolerance = 1e-9;

void test_clamp(Tests::Context& context)
{
	TEST_EXPECT_NEAR(context, Common::limit(-2.0, -1.0, 1.0), -1.0, kTolerance);
	TEST_EXPECT_NEAR(context, Common::limit(0.25, -1.0, 1.0), 0.25, kTolerance);
	TEST_EXPECT_NEAR(context, Common::limit(2.0, -1.0, 1.0), 1.0, kTolerance);
}

void test_actuator(Tests::Context& context)
{
	TEST_EXPECT_NEAR(context, Common::actuator(0.0, 1.0, -0.2, 0.2), 0.2, kTolerance);
	TEST_EXPECT_NEAR(context, Common::actuator(1.0, 0.0, -0.2, 0.2), 0.8, kTolerance);
	TEST_EXPECT_NEAR(context, Common::actuator(0.9, 1.0, -0.2, 0.2), 1.0, kTolerance);
}

void test_units(Tests::Context& context)
{
	TEST_EXPECT_NEAR(context, Common::deg(Common::kPi), 180.0, kTolerance);
	TEST_EXPECT_NEAR(context, Common::rad(180.0), Common::kPi, kTolerance);
}

void test_table_interpolation(Tests::Context& context)
{
	double x[] = { 0.0, 1.0, 2.0 };
	double values[] = { 0.0, 10.0, 20.0 };
	TEST_EXPECT_NEAR(context, Common::lerp(x, values, 3, -1.0), 0.0, kTolerance);
	TEST_EXPECT_NEAR(context, Common::lerp(x, values, 3, 0.5), 5.0, kTolerance);
	TEST_EXPECT_NEAR(context, Common::lerp(x, values, 3, 3.0), 20.0, kTolerance);
}
}

void run_common_tests(Tests::Context& context)
{
	test_clamp(context);
	test_actuator(context);
	test_units(context);
	test_table_interpolation(context);
}
