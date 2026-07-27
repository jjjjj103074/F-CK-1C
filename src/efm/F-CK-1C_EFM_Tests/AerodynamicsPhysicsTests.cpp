#include "TestHarness.h"

#include "Core/Simulation/Models/Aerodynamics/AerodynamicsConfig.h"
#include "Core/Simulation/Models/Aerodynamics/AerodynamicsPhysics.h"

namespace
{
namespace AerodynamicsPhysics =
	Core::Simulation::AerodynamicsPhysics;

constexpr double kTolerance = 1e-9;

void test_conditions_and_primary_forces(Tests::Context& context)
{
	const Core::Simulation::AerodynamicsConfig& config =
		Core::Simulation::fck1c_aerodynamics_config();
	AerodynamicsPhysics::AerodynamicsState state;
	AerodynamicsPhysics::update_aerodynamic_conditions(
		state, config, { Common::Vec3(), 1.225, 100.0, 0.0, 5.0, 0.0, 0.0 });
	TEST_EXPECT_NEAR(context, state.dynamic_pressure, 6125.0, kTolerance);
	TEST_EXPECT_NEAR(context, state.wing_lift_coefficient, 0.4085, kTolerance);
	AerodynamicsPhysics::AerodynamicsFrameInput input;
	int force_count = 0;
	AerodynamicsPhysics::apply_primary_aerodynamics(
		state,
		{ config, input },
		[&force_count](const Common::Vec3&, const Common::Vec3&) { ++force_count; });
	TEST_EXPECT(context, force_count == 7);
}

void test_limiter_sinks(Tests::Context& context)
{
	const Core::Simulation::AerodynamicsConfig& config =
		Core::Simulation::fck1c_aerodynamics_config();
	AerodynamicsPhysics::AerodynamicsState state;
	state.dynamic_pressure = 1000.0;
	state.roll_rate_max = 1.0;
	AerodynamicsPhysics::AerodynamicsFrameInput input;
	int force_count = 0;
	int moment_count = 0;
	auto force_sink = [&force_count](const Common::Vec3&, const Common::Vec3&)
	{
		++force_count;
	};
	auto moment_sink = [&moment_count](const Common::Vec3&) { ++moment_count; };
	AerodynamicsPhysics::apply_aerodynamic_limiters(
		state,
		{ config, input },
		AerodynamicsPhysics::make_aerodynamic_sinks(
			force_sink, moment_sink));
	TEST_EXPECT(context, force_count == 1);
	TEST_EXPECT(context, moment_count == 4);
}
}

void run_aerodynamics_physics_tests(Tests::Context& context)
{
	test_conditions_and_primary_forces(context);
	test_limiter_sinks(context);
}
