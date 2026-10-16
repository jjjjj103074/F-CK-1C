#include "TestHarness.h"

#include "Common/Actuator.h"
#include "Common/Clamp.h"
#include "Common/CommandValue.h"
#include "Common/Interpolation.h"
#include "Common/PathUtils.h"
#include "Common/Table.h"
#include "Common/Units.h"

#include <limits>
#include <string>

namespace
{
constexpr double kTolerance = 1e-9;
constexpr double kNegativeScale = -2.0;
constexpr double kPositiveScale = 3.0;
constexpr double kDefaultPressedValue = 0.75;
constexpr double kCustomPressThreshold = 0.8;

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

void test_command_value_press_threshold(Tests::Context& context)
{
	TEST_EXPECT(context, Common::command_value_is_pressed(kDefaultPressedValue));
	TEST_EXPECT(context, !Common::command_value_is_pressed(kDefaultPressedValue, kCustomPressThreshold));
	TEST_EXPECT(context, !Common::command_value_is_pressed(kCustomPressThreshold, kCustomPressThreshold));
}

void test_rescale_rejects_non_finite_values(Tests::Context& context)
{
	const double invalid = std::numeric_limits<double>::quiet_NaN();
	TEST_EXPECT_NEAR(context, Common::rescale(-0.5, kNegativeScale, kPositiveScale), -1.0, kTolerance);
	TEST_EXPECT_NEAR(context, Common::rescale(0.5, kNegativeScale, kPositiveScale), 1.5, kTolerance);
	TEST_EXPECT(context, rescale_throws(invalid, kNegativeScale, kPositiveScale));
	TEST_EXPECT(context, rescale_throws(0.5, invalid, kPositiveScale));
	TEST_EXPECT(context, rescale_throws(0.5, kNegativeScale, invalid));
}

void test_table_interpolation(Tests::Context& context)
{
	double x[] = { 0.0, 1.0, 2.0 };
	double values[] = { 0.0, 10.0, 20.0 };
	const Common::LinearTable table = { x, values, 3 };
	TEST_EXPECT_NEAR(context, Common::lerp(table, -1.0), 0.0, kTolerance);
	TEST_EXPECT_NEAR(context, Common::lerp(table, 0.5), 5.0, kTolerance);
	TEST_EXPECT_NEAR(context, Common::lerp(table, 3.0), 20.0, kTolerance);
	TEST_EXPECT_NEAR(context, Common::lerp(table, std::numeric_limits<double>::quiet_NaN()), 20.0, kTolerance);
}

void test_path_join(Tests::Context& context)
{
	TEST_EXPECT(context, Common::build_path({ "C:/DCS/Mods", "FM/config.lua" }) == "C:\\DCS\\Mods\\FM\\config.lua");
	TEST_EXPECT(context, Common::build_path({ ".", "FM/config.lua" }) == "FM\\config.lua");
	TEST_EXPECT(context, Common::build_path({ nullptr, "FM/config.lua" }) == "FM\\config.lua");
	TEST_EXPECT(context, Common::build_path({ "", "FM/config.lua" }) == "FM\\config.lua");
	TEST_EXPECT(context, Common::build_path({ "C:/DCS/Mods/", "FM/config.lua" }) == "C:\\DCS\\Mods\\FM\\config.lua");
	TEST_EXPECT(context, Common::build_path({ "C:\\DCS\\Mods\\", "FM/config.lua" }) == "C:\\DCS\\Mods\\FM\\config.lua");
	TEST_EXPECT(context, Common::build_path({ "C:/DCS/Mods", nullptr }) == "C:\\DCS\\Mods\\");
}

void test_path_join_preserves_long_paths(Tests::Context& context)
{
	const std::string component(1200, 'x');
	const std::string root = "C:/" + component;
	const std::string path = Common::build_path({ root.c_str(), "FM/config.lua" });
	TEST_EXPECT(context, path == "C:\\" + component + "\\FM\\config.lua");
}

void test_path_directory_rules(Tests::Context& context)
{
	std::string path = "C:/DCS/Mods/FM/config.lua";
	Common::path_dirname(path);
	TEST_EXPECT(context, path == "C:\\DCS\\Mods\\FM");
	path = "config.lua";
	Common::path_dirname(path);
	TEST_EXPECT(context, path == ".");
	path = "C:/config.lua";
	Common::path_dirname(path);
	TEST_EXPECT(context, path == "C:\\");
	path = "/config.lua";
	Common::path_dirname(path);
	TEST_EXPECT(context, path == "\\");
	path = "C:/DCS/Mods/";
	Common::path_dirname(path);
	TEST_EXPECT(context, path == "C:\\DCS");
	TEST_EXPECT(context, Common::path_has_component_suffix(path, "dcs"));
	TEST_EXPECT(context, !Common::path_has_component_suffix(path, "cs"));
}
}

void run_common_tests(Tests::Context& context)
{
	test_clamp(context);
	test_actuator(context);
	test_units(context);
	test_command_value_press_threshold(context);
	test_rescale_rejects_non_finite_values(context);
	test_table_interpolation(context);
	test_path_join(context);
	test_path_join_preserves_long_paths(context);
	test_path_directory_rules(context);
}
