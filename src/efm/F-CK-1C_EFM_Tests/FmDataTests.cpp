#include "TestHarness.h"

#include "Core/AircraftState.h"
#include "FM_data.h"

namespace FM
{
Core::AircraftState aircraft_state;
}

namespace
{
constexpr double kTolerance = 1e-9;

void test_aircraft_geometry(Tests::Context& context)
{
	TEST_EXPECT_NEAR(context, FM_DATA::wing_area, 24.26, kTolerance);
	TEST_EXPECT_NEAR(context, FM_DATA::wingspan, 8.53, kTolerance);
	TEST_EXPECT_NEAR(context, FM_DATA::length, 14.48, kTolerance);
	TEST_EXPECT_NEAR(context, FM_DATA::height, 4.7, kTolerance);
}

void test_aerodynamic_tables(Tests::Context& context)
{
	TEST_EXPECT(context, FM_DATA::kAeroTableSize == 6);
	TEST_EXPECT_NEAR(context, FM_DATA::mach_table[0], 0.0, kTolerance);
	TEST_EXPECT_NEAR(context, FM_DATA::mach_table[5], 1.5, kTolerance);
	TEST_EXPECT_NEAR(context, FM_DATA::cx0[0], 0.025, kTolerance);
	TEST_EXPECT_NEAR(context, FM_DATA::CyMax[5], 0.6, kTolerance);
}

void test_engine_tables(Tests::Context& context)
{
	TEST_EXPECT(context, FM_DATA::kEngineTableSize == 11);
	TEST_EXPECT_NEAR(context, FM_DATA::engine_mach_table[0], 0.0, kTolerance);
	TEST_EXPECT_NEAR(context, FM_DATA::engine_mach_table[10], 1.0, kTolerance);
	TEST_EXPECT_NEAR(context, FM_DATA::max_thrust[0], 54000.0, kTolerance);
	TEST_EXPECT_NEAR(context, FM_DATA::max_thrust[10], 46600.0, kTolerance);
	TEST_EXPECT_NEAR(context, FM_DATA::engine_power_table[10], 1.0, kTolerance);
}
}

void run_fm_data_tests(Tests::Context& context)
{
	test_aircraft_geometry(context);
	test_aerodynamic_tables(context);
	test_engine_tables(context);
}
