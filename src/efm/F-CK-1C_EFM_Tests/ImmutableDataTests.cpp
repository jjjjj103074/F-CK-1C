#include "TestHarness.h"

#include "Data/AeroTables.h"
#include "Data/AircraftConfig.h"
#include "Data/EngineTables.h"

#include <array>
#include <type_traits>
#include <vector>

namespace
{
constexpr double kTolerance = 1e-9;

const Data::AeroTable kMach = { 0.0, 0.4, 0.6, 0.8, 0.9, 1.5 };
const Data::AeroTable kCxZero = { 0.025, 0.025, 0.0272, 0.048, 0.0741, 0.0741 };
const Data::AeroTable kCyAlpha = { 0.0817, 0.0817, 0.0872, 0.0816, 0.08, 0.08 };
const Data::AeroTable kRollRateMax = { 0.5, 1.5, 2.5, 3.5, 3.5, 3.5 };
const Data::AeroTable kAlphaMax = { 20.0, 20.0, 20.0, 18.0, 15.0, 10.0 };
const Data::AeroTable kCyMax = { 1.21, 1.21, 1.26, 0.755, 0.6, 0.6 };

const Data::EngineTable kEngineMach = {
	0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0
};
const Data::EngineTable kMaxThrust = {
	54000.0, 53600.0, 53200.0, 52800.0, 52300.0, 51600.0,
	50800.0, 49900.0, 48900.0, 47800.0, 46600.0
};
const Data::EngineTable kThrottleInput = {
	0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0
};
const Data::EngineTable kEnginePower = {
	0.0, 0.01, 0.02, 0.06, 0.08, 0.1, 0.3, 0.5, 0.7, 0.9, 1.0
};

static_assert(
	std::is_same<decltype(Data::fck1c_aero_tables()), const Data::AeroTables&>::value,
	"Aerodynamic data interface must be read-only.");
static_assert(
	std::is_same<decltype(Data::fck1c_engine_tables()), const Data::EngineTables&>::value,
	"Engine data interface must be read-only.");
static_assert(
	std::is_same<decltype(Data::fck1c_aircraft_config()), const Data::AircraftConfig&>::value,
	"Aircraft config interface must be read-only.");

template <std::size_t Size>
void expect_table(
	Tests::Context& context,
	const std::array<double, Size>& actual,
	const std::array<double, Size>& expected)
{
	for (std::size_t index = 0; index < Size; ++index)
	{
		TEST_EXPECT_NEAR(context, actual[index], expected[index], kTolerance);
	}
}

template <std::size_t Size>
void expect_owned_table(
	Tests::Context& context,
	const std::vector<double>& actual,
	const std::array<double, Size>& expected)
{
	TEST_EXPECT(context, actual.size() == Size);
	if (actual.size() != Size)
	{
		return;
	}
	for (std::size_t index = 0; index < Size; ++index)
	{
		TEST_EXPECT_NEAR(context, actual[index], expected[index], kTolerance);
	}
}

void test_aircraft_geometry(Tests::Context& context)
{
	const Data::AircraftConfig& config = Data::fck1c_aircraft_config();
	TEST_EXPECT_NEAR(context, config.aerodynamics.wing_area, 24.26, kTolerance);
	TEST_EXPECT_NEAR(context, config.aerodynamics.wingspan, 8.53, kTolerance);
	TEST_EXPECT_NEAR(context, config.aerodynamics.length, 14.48, kTolerance);
	TEST_EXPECT_NEAR(context, config.aerodynamics.height, 4.7, kTolerance);
	TEST_EXPECT_NEAR(context, config.aerodynamics.mach_max, 1.8, kTolerance);
	TEST_EXPECT_NEAR(context, config.left_engine_position.x, -3.793, kTolerance);
	TEST_EXPECT_NEAR(context, config.left_engine_position.z, -0.716, kTolerance);
	TEST_EXPECT_NEAR(context, config.right_engine_position.z, 0.716, kTolerance);
}

void test_aerodynamic_scalars(Tests::Context& context)
{
	const Data::AeroTables& data = Data::fck1c_aero_tables();
	TEST_EXPECT_NEAR(context, data.cy_zero, 0.0001, kTolerance);
	TEST_EXPECT_NEAR(context, data.cz_beta, -0.016, kTolerance);
	TEST_EXPECT_NEAR(context, data.cx_gear, 0.012, kTolerance);
	TEST_EXPECT_NEAR(context, data.cx_airbrake, 0.06, kTolerance);
	TEST_EXPECT_NEAR(context, data.cx_flap, 0.05, kTolerance);
	TEST_EXPECT_NEAR(context, data.cx_lift_k, 0.030, kTolerance);
	TEST_EXPECT_NEAR(context, data.cx_alpha_k, 0.080, kTolerance);
	TEST_EXPECT_NEAR(context, data.cx_elevator_k, 0.008, kTolerance);
	TEST_EXPECT_NEAR(context, data.cy_flap, 0.3, kTolerance);
	TEST_EXPECT_NEAR(context, data.airbrake_pitch_comp_k, 0.003, kTolerance);
}

void test_aerodynamic_tables(Tests::Context& context)
{
	const Data::AeroTables& data = Data::fck1c_aero_tables();
	expect_table(context, data.mach, kMach);
	expect_table(context, data.cx_zero, kCxZero);
	expect_table(context, data.cy_alpha, kCyAlpha);
	expect_table(context, data.roll_rate_max, kRollRateMax);
	expect_table(context, data.alpha_max, kAlphaMax);
	expect_table(context, data.cy_max, kCyMax);
}

void test_engine_data(Tests::Context& context)
{
	const Data::EngineTables& data = Data::fck1c_engine_tables();
	TEST_EXPECT_NEAR(context, data.fuel_consumption, 0.37, kTolerance);
	TEST_EXPECT_NEAR(context, data.start_time, 60.0, kTolerance);
	TEST_EXPECT_NEAR(context, data.spool_up_tau, 2.5, kTolerance);
	TEST_EXPECT_NEAR(context, data.spool_down_tau, 4.0, kTolerance);
	expect_table(context, data.mach, kEngineMach);
	expect_table(context, data.max_thrust, kMaxThrust);
	expect_table(context, data.throttle_input, kThrottleInput);
	expect_table(context, data.power, kEnginePower);
}

void test_config_injection(Tests::Context& context)
{
	const Data::AircraftConfig& config = Data::fck1c_aircraft_config();
	const Data::AeroTables& aero = Data::fck1c_aero_tables();
	const Data::EngineTables& engine = Data::fck1c_engine_tables();
	expect_owned_table(context, config.aerodynamics.mach_table, aero.mach);
	expect_owned_table(context, config.aerodynamics.cy_max_table, aero.cy_max);
	expect_owned_table(context, config.engine.mach_table, engine.mach);
	expect_owned_table(context, config.engine.max_thrust_table, engine.max_thrust);
	expect_owned_table(context, config.engine.throttle_input_table, engine.throttle_input);
	expect_owned_table(context, config.engine.power_table, engine.power);
	TEST_EXPECT_NEAR(context, Systems::max_dry_thrust(config.engine, 0.1), 53600.0, kTolerance);
	TEST_EXPECT_NEAR(context, config.fuel.consumption_rate, 0.37, kTolerance);
}
}

void run_immutable_data_tests(Tests::Context& context)
{
	test_aircraft_geometry(context);
	test_aerodynamic_scalars(context);
	test_aerodynamic_tables(context);
	test_engine_data(context);
	test_config_injection(context);
}
