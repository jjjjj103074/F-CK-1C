#include "TestHarness.h"

#include "Core/Simulation/Models/Aerodynamics/AerodynamicsConfig.h"
#include "Core/Simulation/Models/Aerodynamics/AerodynamicsModel.h"
#include "Core/Simulation/Models/GroundInteraction/GroundInteractionConfig.h"
#include "Core/Simulation/Models/GroundInteraction/GroundInteractionModel.h"
#include "Core/Simulation/Models/Propulsion/PropulsionConfig.h"
#include "Core/Simulation/Models/Propulsion/PropulsionModel.h"
#include "Core/Systems/Engine/Engine.h"
#include "Core/Systems/LandingGear/LandingGear.h"
#include "Core/Systems/LandingGear/LandingGearConfig.h"
#include "Common/Units.h"

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
	TEST_EXPECT_NEAR(context, config.wing_area_m2, 24.26, kTolerance);
	TEST_EXPECT_NEAR(context, config.wingspan_m, 8.53, kTolerance);
	TEST_EXPECT_NEAR(context, config.length_m, 14.48, kTolerance);
	TEST_EXPECT_NEAR(context, config.height_m, 4.7, kTolerance);
	TEST_EXPECT_NEAR(context, config.mach_max, 1.8, kTolerance);
	TEST_EXPECT_NEAR(context, config.cy_zero, 0.0001, kTolerance);
	TEST_EXPECT_NEAR(context, config.cz_beta, -0.016, kTolerance);
	TEST_EXPECT_NEAR(context, config.cx_gear, 0.012, kTolerance);
	TEST_EXPECT_NEAR(context, config.cx_airbrake, 0.06, kTolerance);
	TEST_EXPECT_NEAR(context, config.cx_flap, 0.05, kTolerance);
	TEST_EXPECT_NEAR(context, config.cx_lift_k, 0.030, kTolerance);
	TEST_EXPECT_NEAR(context, config.cx_alpha_k, 0.080, kTolerance);
	TEST_EXPECT_NEAR(
		context,
		config.cx_stabilator_per_rad,
		0.008 / Common::rad(25.0),
		kTolerance);
	TEST_EXPECT_NEAR(context, config.cy_flap, 0.3, kTolerance);
	TEST_EXPECT_NEAR(
		context,
		config.airbrake_pitch_moment_coefficient,
		0.003,
		kTolerance);
	expect_table(context, config.mach_table, kAerodynamicMach);
	expect_table(context, config.cx_zero_table, kCxZero);
	expect_table(context, config.cy_alpha_table, kCyAlpha);
	expect_table(context, config.alpha_max_table_deg, kAlphaMax);
	expect_table(context, config.cy_max_table, kCyMax);
}

void test_engine_production_config(Tests::Context& context)
{
	const auto& engine = Core::Systems::fck1c_engine_config();
	TEST_EXPECT_NEAR(
		context, engine.fuel_consumption_rate_kg_s, 0.37, kTolerance);
	TEST_EXPECT_NEAR(context, engine.start_time_s, 60.0, kTolerance);
	TEST_EXPECT_NEAR(context, engine.spool_up_tau_s, 2.5, kTolerance);
	TEST_EXPECT_NEAR(context, engine.spool_down_tau_s, 4.0, kTolerance);
	expect_table(
		context, engine.throttle_input_table_normalized, kEngineMach);
	expect_table(context, engine.power_table_normalized, kEnginePower);
	TEST_EXPECT_NEAR(
		context, engine.afterburner.detent_normalized, 0.70, kTolerance);
	TEST_EXPECT_NEAR(
		context, engine.afterburner.fuel_factor, 2.2, kTolerance);
	TEST_EXPECT_NEAR(
		context, engine.afterburner.core_rpm_0_1, 0.94, kTolerance);
	TEST_EXPECT_NEAR(
		context, engine.afterburner.core_drop_time_s, 0.80, kTolerance);
	TEST_EXPECT_NEAR(
		context, engine.afterburner.spool_in_tau_s, 2.0, kTolerance);
	TEST_EXPECT_NEAR(
		context, engine.afterburner.spool_out_tau_s, 0.6, kTolerance);
	TEST_EXPECT_NEAR(
		context,
		engine.afterburner.light_throttle_output_min_normalized,
		0.88,
		kTolerance);
}

void test_propulsion_production_config(Tests::Context& context)
{
	const auto& propulsion =
		Core::Simulation::fck1c_propulsion_config();
	expect_table(context, propulsion.mach_table, kEngineMach);
	expect_table(context, propulsion.max_thrust_table_n, kMaxThrust);
	TEST_EXPECT_NEAR(
		context, propulsion.afterburner_thrust_factor, 1.73, kTolerance);
	TEST_EXPECT_NEAR(
		context,
		Core::Simulation::fck1c_carrier_launch_reference_thrust_n(),
		53600.0,
		kTolerance);
	TEST_EXPECT_NEAR(
		context, propulsion.left_engine_position_body_m.x, -3.793, kTolerance);
	TEST_EXPECT_NEAR(
		context, propulsion.left_engine_position_body_m.y, -0.391, kTolerance);
	TEST_EXPECT_NEAR(
		context, propulsion.left_engine_position_body_m.z, -0.716, kTolerance);
	TEST_EXPECT_NEAR(
		context, propulsion.right_engine_position_body_m.x, -3.793, kTolerance);
	TEST_EXPECT_NEAR(
		context, propulsion.right_engine_position_body_m.y, -0.391, kTolerance);
	TEST_EXPECT_NEAR(
		context, propulsion.right_engine_position_body_m.z, 0.716, kTolerance);
}

void test_ground_interaction_production_config(Tests::Context& context)
{
	const auto& config =
		Core::Simulation::fck1c_ground_interaction_config();
	expect_vec3(context, config.gear_points_body_m[0], { 4.12, -1.912, 0.0 });
	expect_vec3(
		context, config.gear_points_body_m[1], { -1.185, -1.913, -0.7905 });
	expect_vec3(
		context, config.gear_points_body_m[2], { -1.185, -1.913, 0.7905 });
	expect_array(context, config.spring_rate_n_m, kGroundSpring);
	expect_array(context, config.damping_n_s_m, kGroundDamping);
	expect_array(context, config.contact_band_m, kGroundContactBand);
	expect_vec3(context, config.belly_point_body_m, { 0.0, -1.05, 0.0 });
	TEST_EXPECT(context, !config.enable_fallback_ground_forces);
}

void test_landing_gear_production_config(Tests::Context& context)
{
	const auto& landing = Core::Systems::fck1c_landing_gear_config();
	expect_array(context, landing.wheel_radius_m, kWheelRadius);
}

void test_engine_owner_rejects_invalid_config(Tests::Context& context)
{
	auto invalid_drop_time = Core::Systems::fck1c_engine_config();
	invalid_drop_time.afterburner.core_drop_time_s = 0.0;
	TEST_EXPECT(context, rejects_invalid_config([invalid_drop_time]()
		{
			(void)Core::Systems::make_engine_system_entry(invalid_drop_time);
		}));
	auto invalid_schedule = Core::Systems::fck1c_engine_config();
	invalid_schedule.throttle_input_table_normalized[1] =
		invalid_schedule.throttle_input_table_normalized[0];
	TEST_EXPECT(context, rejects_invalid_config([invalid_schedule]()
		{
			(void)Core::Systems::make_engine_system_entry(invalid_schedule);
		}));
}

void test_landing_owner_rejects_invalid_config(Tests::Context& context)
{
	auto config = Core::Systems::fck1c_landing_gear_config();
	config.wheel_radius_m[0] = std::numeric_limits<double>::quiet_NaN();
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
	invalid_alpha.alpha_max_table_deg[0] = 0.0;
	TEST_EXPECT(context, rejects_invalid_config([invalid_alpha]()
		{
			Core::Simulation::AerodynamicsModel model(invalid_alpha);
		}));
	auto invalid_airbrake = Core::Simulation::fck1c_aerodynamics_config();
	invalid_airbrake.airbrake_pitch_moment_coefficient =
		std::numeric_limits<double>::quiet_NaN();
	TEST_EXPECT(context, rejects_invalid_config([invalid_airbrake]()
		{
			Core::Simulation::AerodynamicsModel model(invalid_airbrake);
		}));
	auto propulsion = Core::Simulation::fck1c_propulsion_config();
	propulsion.mach_table[1] = propulsion.mach_table[0];
	TEST_EXPECT(context, rejects_invalid_config([propulsion]()
		{
			Core::Simulation::PropulsionModel model(propulsion);
		}));
	auto ground = Core::Simulation::fck1c_ground_interaction_config();
	ground.spring_rate_n_m[0] = std::numeric_limits<double>::quiet_NaN();
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
	test_landing_gear_production_config(context);
	test_engine_owner_rejects_invalid_config(context);
	test_landing_owner_rejects_invalid_config(context);
	test_model_owners_reject_invalid_config(context);
}
