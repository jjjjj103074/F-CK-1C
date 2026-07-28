#include "TestHarness.h"

#include "Common/Actuator.h"
#include "Common/Clamp.h"
#include "Common/Interpolation.h"
#include "Common/PathUtils.h"
#include "Common/Table.h"
#include "Common/Units.h"

#include <cstring>
#include <limits>

namespace
{
constexpr double kTolerance = 1e-9;
constexpr double kNegativeScale = -2.0;
constexpr double kPositiveScale = 3.0;

bool rescale_throws(double input, double minimum, double maximum)
{
	try
	{
		(void)Common::rescale(input, minimum, maximum);
	}
	catch (const std::domain_error&)
	{
		return true;
	}
	return false;
}

void test_clamp(Tests::Context& context)
{
	TEST_EXPECT_NEAR(context, Common::limit(-2.0, -1.0, 1.0), -1.0, kTolerance);
	TEST_EXPECT_NEAR(context, Common::limit(0.25, -1.0, 1.0), 0.25, kTolerance);
	TEST_EXPECT_NEAR(context, Common::limit(2.0, -1.0, 1.0), 1.0, kTolerance);
}

void test_actuator(Tests::Context& context)
{
	TEST_EXPECT_NEAR(context, Common::actuator(0.0, { 1.0, -0.2, 0.2 }), 0.2, kTolerance);
	TEST_EXPECT_NEAR(context, Common::actuator(1.0, { 0.0, -0.2, 0.2 }), 0.8, kTolerance);
	TEST_EXPECT_NEAR(context, Common::actuator(0.9, { 1.0, -0.2, 0.2 }), 1.0, kTolerance);
}

void test_units(Tests::Context& context)
{
	TEST_EXPECT_NEAR(context, Common::deg(Common::kPi), 180.0, kTolerance);
	TEST_EXPECT_NEAR(context, Common::rad(180.0), Common::kPi, kTolerance);
}

void test_rescale_rejects_non_finite_values(Tests::Context& context)
{
	const double invalid = std::numeric_limits<double>::quiet_NaN();
	TEST_EXPECT_NEAR(
		context,
		Common::rescale(-0.5, kNegativeScale, kPositiveScale),
		-1.0,
		kTolerance);
	TEST_EXPECT_NEAR(
		context,
		Common::rescale(0.5, kNegativeScale, kPositiveScale),
		1.5,
		kTolerance);
	TEST_EXPECT(context, rescale_throws(
		invalid, kNegativeScale, kPositiveScale));
	TEST_EXPECT(context, rescale_throws(
		0.5, invalid, kPositiveScale));
	TEST_EXPECT(context, rescale_throws(
		0.5, kNegativeScale, invalid));
}

void test_table_interpolation(Tests::Context& context)
{
	double x[] = { 0.0, 1.0, 2.0 };
	double values[] = { 0.0, 10.0, 20.0 };
	const Common::LinearTable table = { x, values, 3 };
	TEST_EXPECT_NEAR(context, Common::lerp(table, -1.0), 0.0, kTolerance);
	TEST_EXPECT_NEAR(context, Common::lerp(table, 0.5), 5.0, kTolerance);
	TEST_EXPECT_NEAR(context, Common::lerp(table, 3.0), 20.0, kTolerance);
	TEST_EXPECT_NEAR(
		context,
		Common::lerp(table, std::numeric_limits<double>::quiet_NaN()),
		20.0,
		kTolerance);
}

void test_path_join(Tests::Context& context)
{
	char path[128];
	Common::build_path({ path, sizeof(path) }, { "C:/DCS/Mods", "FM/config.lua" });
	TEST_EXPECT(context, std::strcmp(path, "C:\\DCS\\Mods\\FM\\config.lua") == 0);
	Common::build_path({ path, sizeof(path) }, { ".", "FM/config.lua" });
	TEST_EXPECT(context, std::strcmp(path, "FM\\config.lua") == 0);
}
}

void run_common_tests(Tests::Context& context)
{
	test_clamp(context);
	test_actuator(context);
	test_units(context);
	test_rescale_rejects_non_finite_values(context);
	test_table_interpolation(context);
	test_path_join(context);
}
