#include "TestHarness.h"

#include "Core/Simulation/Models/Aerodynamics/AerodynamicsConfig.h"
#include "Core/Simulation/Models/Aerodynamics/AerodynamicsPhysics.h"

#include <cmath>

namespace
{
namespace AerodynamicsPhysics =
	Core::Simulation::AerodynamicsPhysics;

constexpr double kTolerance = 1e-9;

struct EasyFlightEffect
{
	Common::Vec3 moment_nm;
	Common::Vec3 side_force_n;
};

EasyFlightEffect easy_flight_effect(
	const Core::Simulation::AerodynamicsConfig& config,
	double surface_ratio)
{
	AerodynamicsPhysics::AerodynamicsState state;
	AerodynamicsPhysics::AerodynamicsFrameInput input;
	input.easy_flight = true;
	input.roll_rate_rad_s = 4.0;
	input.differential_flaperon_position_rad =
		surface_ratio * config.easy_flight_flaperon_limit_rad;
	input.rudder_position_rad =
		surface_ratio * config.easy_flight_rudder_limit_rad;
	EasyFlightEffect result;
	auto force = [&result](const Common::Vec3& value, const Common::Vec3&)
	{
		result.side_force_n = value;
	};
	auto moment = [&result](const Common::Vec3& value)
	{
		result.moment_nm = value;
	};
	AerodynamicsPhysics::apply_supplemental_aerodynamics(
		state, { config, input },
		AerodynamicsPhysics::make_aerodynamic_sinks(force, moment));
	return result;
}

void test_conditions_and_primary_forces(Tests::Context& context)
{
	const Core::Simulation::AerodynamicsConfig& config =
		Core::Simulation::fck1c_aerodynamics_config();
	AerodynamicsPhysics::AerodynamicsState state;
	AerodynamicsPhysics::update_aerodynamic_conditions(
		state, config, { Common::Vec3(), 1.225, 100.0, 0.0, 5.0, 0.0, 0.0 });
	TEST_EXPECT_NEAR(context, state.dynamic_pressure_pa, 6125.0, kTolerance);
	TEST_EXPECT_NEAR(context, state.wing_lift_coefficient, 0.4085, kTolerance);
	AerodynamicsPhysics::AerodynamicsFrameInput input;
	int force_count = 0;
	AerodynamicsPhysics::apply_primary_aerodynamics(
		state,
		{ config, input },
		[&force_count](const Common::Vec3&, const Common::Vec3&) { ++force_count; });
	TEST_EXPECT(context, force_count == 7);
}

void test_supplemental_aerodynamics_are_explicit(Tests::Context& context)
{
	const Core::Simulation::AerodynamicsConfig& config =
		Core::Simulation::fck1c_aerodynamics_config();
	AerodynamicsPhysics::AerodynamicsState state;
	state.dynamic_pressure_pa = 1000.0;
	AerodynamicsPhysics::AerodynamicsFrameInput input;
	int force_count = 0;
	int moment_count = 0;
	auto force_sink = [&force_count](const Common::Vec3&, const Common::Vec3&)
	{
		++force_count;
	};
	auto moment_sink = [&moment_count](const Common::Vec3&) { ++moment_count; };
	AerodynamicsPhysics::apply_supplemental_aerodynamics(
		state,
		{ config, input },
		AerodynamicsPhysics::make_aerodynamic_sinks(
			force_sink, moment_sink));
	TEST_EXPECT(context, force_count == 0);
	TEST_EXPECT(context, moment_count == 1);
	input.easy_flight = true;
	AerodynamicsPhysics::apply_supplemental_aerodynamics(
		state,
		{ config, input },
		AerodynamicsPhysics::make_aerodynamic_sinks(
			force_sink, moment_sink));
	TEST_EXPECT(context, force_count == 2);
	TEST_EXPECT(context, moment_count == 3);
}

void test_directional_aerodynamics_oppose_positive_yaw(
	Tests::Context& context)
{
	const auto& config = Core::Simulation::fck1c_aerodynamics_config();
	AerodynamicsPhysics::AerodynamicsState state;
	state.dynamic_pressure_pa = 1000.0;
	state.rudder_position_body_m = { -1.0, 0.0, 0.0 };
	Common::Vec3 yaw_damping_force;
	AerodynamicsPhysics::AerodynamicsFrameInput yaw_input;
	yaw_input.yaw_rate_rad_s = 0.2;
	AerodynamicsPhysics::apply_rudder_aerodynamics(
		state,
		{ config, yaw_input },
		[&yaw_damping_force](const Common::Vec3& force, const Common::Vec3&)
		{ yaw_damping_force = force; });
	TEST_EXPECT(context, yaw_damping_force.z < 0.0);
	Common::Vec3 right_rudder_force;
	AerodynamicsPhysics::AerodynamicsFrameInput rudder_input;
	rudder_input.rudder_position_rad = -0.2;
	AerodynamicsPhysics::apply_rudder_aerodynamics(
		state,
		{ config, rudder_input },
		[&right_rudder_force](const Common::Vec3& force, const Common::Vec3&)
		{ right_rudder_force = force; });
	TEST_EXPECT(context, right_rudder_force.z < 0.0);
}

void test_easy_flight_surface_calibration_uses_dimensionless_ratios(
	Tests::Context& context)
{
	const auto& config = Core::Simulation::fck1c_aerodynamics_config();
	const EasyFlightEffect zero = easy_flight_effect(config, 0.0);
	const EasyFlightEffect half = easy_flight_effect(config, 0.5);
	const EasyFlightEffect full = easy_flight_effect(config, 1.0);
	TEST_EXPECT_NEAR(context, zero.moment_nm.x, -100000.0, kTolerance);
	TEST_EXPECT_NEAR(context, half.moment_nm.x,
		-100000.0 * (1.0 - std::sqrt(0.5)), kTolerance);
	TEST_EXPECT_NEAR(context, full.moment_nm.x, 0.0, kTolerance);
	TEST_EXPECT_NEAR(context, zero.side_force_n.z, 0.0, kTolerance);
	TEST_EXPECT_NEAR(context, half.side_force_n.z, -50000.0, kTolerance);
	TEST_EXPECT_NEAR(context, full.side_force_n.z, -100000.0, kTolerance);
}
}

void run_aerodynamics_physics_tests(Tests::Context& context)
{
	test_conditions_and_primary_forces(context);
	test_supplemental_aerodynamics_are_explicit(context);
	test_directional_aerodynamics_oppose_positive_yaw(context);
	test_easy_flight_surface_calibration_uses_dimensionless_ratios(context);
}
