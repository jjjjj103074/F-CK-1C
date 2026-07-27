#include "TestHarness.h"

#include "Core/Simulation/Models/Aerodynamics/AerodynamicsConfig.h"
#include "Core/Simulation/Models/Aerodynamics/AerodynamicsModel.h"
#include "Core/Simulation/Models/GroundInteraction/GroundInteractionConfig.h"
#include "Core/Simulation/Models/GroundInteraction/GroundInteractionModel.h"
#include "Core/Simulation/Models/Propulsion/PropulsionConfig.h"
#include "Core/Simulation/Models/Propulsion/PropulsionModel.h"
#include "Core/Systems/Engine/Engine.h"
#include "Core/Systems/FlightControlComputer/FlightControlComputer.h"
#include "Core/Systems/FlightControlComputer/FlightControlComputerConfig.h"
#include "Core/Systems/LandingGear/LandingGear.h"
#include "Core/Systems/LandingGear/LandingGearConfig.h"

#include <array>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace
{
constexpr double kTolerance = 1e-9;

const std::array<double, 6> kAerodynamicMach = {
	0.0, 0.4, 0.6, 0.8, 0.9, 1.5
};
const std::array<double, 6> kAlphaMax = {
	20.0, 20.0, 20.0, 18.0, 15.0, 10.0
};
const std::array<double, 6> kCxZero = {
	0.025, 0.025, 0.0272, 0.048, 0.0741, 0.0741
};
const std::array<double, 6> kCyAlpha = {
	0.0817, 0.0817, 0.0872, 0.0816, 0.08, 0.08
};
const std::array<double, 6> kRollRateMax = {
	0.5, 1.5, 2.5, 3.5, 3.5, 3.5
};
const std::array<double, 6> kCyMax = {
	1.21, 1.21, 1.26, 0.755, 0.6, 0.6
};
const std::array<double, 11> kEngineMach = {
	0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0
};
const std::array<double, 11> kMaxThrust = {
	54000.0, 53600.0, 53200.0, 52800.0, 52300.0, 51600.0,
	50800.0, 49900.0, 48900.0, 47800.0, 46600.0
};
const std::array<double, 11> kEnginePower = {
	0.0, 0.01, 0.02, 0.06, 0.08, 0.1, 0.3, 0.5, 0.7, 0.9, 1.0
};
const std::array<double, 3> kWheelRadius = {
	0.2286, 0.3048, 0.3048
};
const std::array<double, 3> kGroundSpring = {
	1000000.0, 3200000.0, 3200000.0
};
const std::array<double, 3> kGroundDamping = {
	12000.0, 20000.0, 20000.0
};
const std::array<double, 3> kGroundContactBand = {
	0.015, 0.055, 0.055
};

static_assert(
	std::is_same<
		decltype(Core::Simulation::fck1c_aerodynamics_config()),
		const Core::Simulation::AerodynamicsConfig&>::value,
	"Aerodynamics production config must be read-only.");
static_assert(
	std::is_same<
		decltype(Core::Simulation::fck1c_propulsion_config()),
		const Core::Simulation::PropulsionConfig&>::value,
	"Propulsion production config must be read-only.");
static_assert(
	std::is_same<
		decltype(Core::Systems::fck1c_engine_config()),
		const Core::Systems::EngineConfig&>::value,
	"Engine production config must be read-only.");
static_assert(
	std::is_same<
		decltype(Core::Simulation::fck1c_ground_interaction_config()),
		const Core::Simulation::GroundInteractionConfig&>::value,
	"Ground-interaction production config must be read-only.");
static_assert(
	std::is_same<
		decltype(Core::Systems::fck1c_flight_control_computer_config()),
		const Core::Systems::FlightControlComputerConfig&>::value,
	"Flight-control production config must be read-only.");
static_assert(
	std::is_same<
		decltype(Core::Systems::fck1c_landing_gear_config()),
		const Core::Systems::LandingGearConfig&>::value,
	"Landing-gear production config must be read-only.");

template <std::size_t Size>
void expect_table(
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

template <std::size_t Size>
void expect_array(
	Tests::Context& context,
	const std::array<double, Size>& actual,
	const std::array<double, Size>& expected)
{
	for (std::size_t index = 0; index < Size; ++index)
	{
		TEST_EXPECT_NEAR(context, actual[index], expected[index], kTolerance);
	}
}

void expect_vec3(
	Tests::Context& context,
	const Common::Vec3& actual,
	const Common::Vec3& expected)
{
	TEST_EXPECT_NEAR(context, actual.x, expected.x, kTolerance);
	TEST_EXPECT_NEAR(context, actual.y, expected.y, kTolerance);
	TEST_EXPECT_NEAR(context, actual.z, expected.z, kTolerance);
}

template <typename Action>
bool rejects_invalid_config(Action action)
{
	try
	{
		action();
	}
	catch (const std::invalid_argument&)
	{
		return true;
	}
	return false;
}

void test_aerodynamics_production_config(Tests::Context& context)
{
	const auto& config = Core::Simulation::fck1c_aerodynamics_config();
	TEST_EXPECT_NEAR(context, config.wing_area, 24.26, kTolerance);
	TEST_EXPECT_NEAR(context, config.wingspan, 8.53, kTolerance);
	TEST_EXPECT_NEAR(context, config.length, 14.48, kTolerance);
	TEST_EXPECT_NEAR(context, config.height, 4.7, kTolerance);
	TEST_EXPECT_NEAR(context, config.mach_max, 1.8, kTolerance);
	TEST_EXPECT_NEAR(context, config.cy_zero, 0.0001, kTolerance);
	TEST_EXPECT_NEAR(context, config.cz_beta, -0.016, kTolerance);
	TEST_EXPECT_NEAR(context, config.cx_gear, 0.012, kTolerance);
	TEST_EXPECT_NEAR(context, config.cx_airbrake, 0.06, kTolerance);
	TEST_EXPECT_NEAR(context, config.cx_flap, 0.05, kTolerance);
	TEST_EXPECT_NEAR(context, config.cx_lift_k, 0.030, kTolerance);
	TEST_EXPECT_NEAR(context, config.cx_alpha_k, 0.080, kTolerance);
	TEST_EXPECT_NEAR(context, config.cx_elevator_k, 0.008, kTolerance);
	TEST_EXPECT_NEAR(context, config.cy_flap, 0.3, kTolerance);
	TEST_EXPECT_NEAR(
		context, config.airbrake_pitch_comp_k, 0.003, kTolerance);
	expect_table(context, config.mach_table, kAerodynamicMach);
	expect_table(context, config.cx_zero_table, kCxZero);
	expect_table(context, config.cy_alpha_table, kCyAlpha);
	expect_table(context, config.roll_rate_max_table, kRollRateMax);
	expect_table(context, config.alpha_max_table, kAlphaMax);
	expect_table(context, config.cy_max_table, kCyMax);
}

void test_engine_production_config(Tests::Context& context)
{
	const auto& engine = Core::Systems::fck1c_engine_config();
	TEST_EXPECT_NEAR(context, engine.fuel_consumption_rate, 0.37, kTolerance);
	TEST_EXPECT_NEAR(context, engine.start_time, 60.0, kTolerance);
	TEST_EXPECT_NEAR(context, engine.spool_up_tau, 2.5, kTolerance);
	TEST_EXPECT_NEAR(context, engine.spool_down_tau, 4.0, kTolerance);
	expect_table(context, engine.throttle_input_table, kEngineMach);
	expect_table(context, engine.power_table, kEnginePower);
	TEST_EXPECT_NEAR(
		context, engine.afterburner.detent, 0.70, kTolerance);
	TEST_EXPECT_NEAR(
		context, engine.afterburner.fuel_factor, 2.2, kTolerance);
	TEST_EXPECT_NEAR(
		context, engine.afterburner.core_rpm, 0.94, kTolerance);
	TEST_EXPECT_NEAR(
		context, engine.afterburner.core_drop_time, 0.80, kTolerance);
	TEST_EXPECT_NEAR(
		context, engine.afterburner.spool_in_tau, 2.0, kTolerance);
	TEST_EXPECT_NEAR(
		context, engine.afterburner.spool_out_tau, 0.6, kTolerance);
	TEST_EXPECT_NEAR(
		context,
		engine.afterburner.light_throttle_output_min,
		0.88,
		kTolerance);
}

void test_propulsion_production_config(Tests::Context& context)
{
	const auto& propulsion =
		Core::Simulation::fck1c_propulsion_config();
	expect_table(context, propulsion.mach_table, kEngineMach);
	expect_table(context, propulsion.max_thrust_table, kMaxThrust);
	TEST_EXPECT_NEAR(
		context, propulsion.afterburner_thrust_factor, 1.73, kTolerance);
	TEST_EXPECT_NEAR(
		context,
		Core::Simulation::fck1c_carrier_launch_reference_thrust(),
		53600.0,
		kTolerance);
	TEST_EXPECT_NEAR(
		context, propulsion.left_engine_position.x, -3.793, kTolerance);
	TEST_EXPECT_NEAR(
		context, propulsion.left_engine_position.y, -0.391, kTolerance);
	TEST_EXPECT_NEAR(
		context, propulsion.left_engine_position.z, -0.716, kTolerance);
	TEST_EXPECT_NEAR(
		context, propulsion.right_engine_position.x, -3.793, kTolerance);
	TEST_EXPECT_NEAR(
		context, propulsion.right_engine_position.y, -0.391, kTolerance);
	TEST_EXPECT_NEAR(
		context, propulsion.right_engine_position.z, 0.716, kTolerance);
}

void test_ground_interaction_production_config(Tests::Context& context)
{
	const auto& config =
		Core::Simulation::fck1c_ground_interaction_config();
	expect_vec3(context, config.gear_points[0], { 4.12, -1.912, 0.0 });
	expect_vec3(context, config.gear_points[1], { -1.185, -1.913, -0.7905 });
	expect_vec3(context, config.gear_points[2], { -1.185, -1.913, 0.7905 });
	expect_array(context, config.spring, kGroundSpring);
	expect_array(context, config.damping, kGroundDamping);
	expect_array(context, config.contact_band, kGroundContactBand);
	expect_vec3(context, config.belly_point, { 0.0, -1.05, 0.0 });
	TEST_EXPECT(context, !config.enable_fallback_ground_forces);
}

void test_fcc_and_landing_gear_production_config(Tests::Context& context)
{
	const auto& fcc =
		Core::Systems::fck1c_flight_control_computer_config();
	const auto& landing = Core::Systems::fck1c_landing_gear_config();
	expect_table(context, fcc.mach_table, kAerodynamicMach);
	expect_table(context, fcc.alpha_limit_deg, kAlphaMax);
	expect_array(context, landing.wheel_radius, kWheelRadius);
}

void test_engine_owner_rejects_invalid_config(Tests::Context& context)
{
	auto invalid_drop_time = Core::Systems::fck1c_engine_config();
	invalid_drop_time.afterburner.core_drop_time = 0.0;
	TEST_EXPECT(context, rejects_invalid_config([invalid_drop_time]()
		{
			(void)Core::Systems::make_engine_system_entry(invalid_drop_time);
		}));
	auto invalid_schedule = Core::Systems::fck1c_engine_config();
	invalid_schedule.throttle_input_table[1] =
		invalid_schedule.throttle_input_table[0];
	TEST_EXPECT(context, rejects_invalid_config([invalid_schedule]()
		{
			(void)Core::Systems::make_engine_system_entry(invalid_schedule);
		}));
}

void test_fcc_owner_rejects_invalid_config(Tests::Context& context)
{
	auto invalid_region =
		Core::Systems::fck1c_flight_control_computer_config();
	invalid_region.control_laws.region_high_kts =
		invalid_region.control_laws.region_low_kts;
	TEST_EXPECT(context, rejects_invalid_config([invalid_region]()
		{
			(void)Core::Systems::make_flight_control_computer_system_entry(
				invalid_region);
		}));
	auto invalid_schedule =
		Core::Systems::fck1c_flight_control_computer_config();
	invalid_schedule.mach_table[1] = invalid_schedule.mach_table[0];
	TEST_EXPECT(context, rejects_invalid_config([invalid_schedule]()
		{
			(void)Core::Systems::make_flight_control_computer_system_entry(
				invalid_schedule);
		}));
}

void test_landing_owner_rejects_invalid_config(Tests::Context& context)
{
	auto config = Core::Systems::fck1c_landing_gear_config();
	config.wheel_radius[0] = std::numeric_limits<double>::quiet_NaN();
	TEST_EXPECT(context, rejects_invalid_config([config]()
		{
			(void)Core::Systems::make_landing_gear_system_entry(config);
		}));
}

void test_model_owners_reject_invalid_config(Tests::Context& context)
{
	auto aerodynamics = Core::Simulation::fck1c_aerodynamics_config();
	aerodynamics.mach_table[1] = aerodynamics.mach_table[0];
	TEST_EXPECT(context, rejects_invalid_config([aerodynamics]()
		{
			Core::Simulation::AerodynamicsModel model(aerodynamics);
		}));
	auto invalid_alpha = Core::Simulation::fck1c_aerodynamics_config();
	invalid_alpha.alpha_max_table[0] = 0.0;
	TEST_EXPECT(context, rejects_invalid_config([invalid_alpha]()
		{
			Core::Simulation::AerodynamicsModel model(invalid_alpha);
		}));
	auto invalid_roll_rate = Core::Simulation::fck1c_aerodynamics_config();
	invalid_roll_rate.roll_rate_max_table[0] = -0.1;
	TEST_EXPECT(context, rejects_invalid_config([invalid_roll_rate]()
		{
			Core::Simulation::AerodynamicsModel model(invalid_roll_rate);
		}));
	auto propulsion = Core::Simulation::fck1c_propulsion_config();
	propulsion.mach_table[1] = propulsion.mach_table[0];
	TEST_EXPECT(context, rejects_invalid_config([propulsion]()
		{
			Core::Simulation::PropulsionModel model(propulsion);
		}));
	auto ground = Core::Simulation::fck1c_ground_interaction_config();
	ground.spring[0] = std::numeric_limits<double>::quiet_NaN();
	TEST_EXPECT(context, rejects_invalid_config([ground]()
		{
			Core::Simulation::GroundInteractionModel model(ground);
		}));
}
}

void run_configuration_ownership_tests(Tests::Context& context)
{
	test_aerodynamics_production_config(context);
	test_engine_production_config(context);
	test_propulsion_production_config(context);
	test_ground_interaction_production_config(context);
	test_fcc_and_landing_gear_production_config(context);
	test_engine_owner_rejects_invalid_config(context);
	test_fcc_owner_rejects_invalid_config(context);
	test_landing_owner_rejects_invalid_config(context);
	test_model_owners_reject_invalid_config(context);
}
