#include "Fck1cEfmTestFixture.h"
#include "TestHarness.h"

#include "Core/Simulation/Models/Aerodynamics/AerodynamicsModel.h"
#include "Core/Simulation/Models/GroundInteraction/GroundInteractionModel.h"
#include "Core/Simulation/Models/Propulsion/PropulsionModel.h"

#include <cmath>

namespace
{
constexpr double kTolerance = 1e-9;
constexpr std::size_t kPrimaryAerodynamicEffectCount = 7;
constexpr std::size_t kNormalLimiterEffectCount = 5;
constexpr std::size_t kEasyFlightLimiterEffectCount = 7;
constexpr double kFullThrottle = 1.0;
constexpr double kDamagedCondition = 0.25;
constexpr double kExpectedFullDryThrustPerEngine = 27000.0;
constexpr double kExpectedDamagedThrust = 6750.0;
constexpr std::size_t kLeftMainWheelIndex = 1;
constexpr std::size_t kFullFallbackEffectCount = 3;
constexpr std::size_t kPartialFallbackEffectCount = 2;
constexpr std::size_t kBellyFallbackEffectCount = 1;
constexpr double kExpectedFullFallbackVerticalForce = 145920.0;
constexpr double kExpectedPartialFallbackVerticalForce = 72960.0;
constexpr double kBellyContactAltitude = 0.5;

double total_vertical_force(
	const Core::Simulation::GroundInteractionResult& result)
{
	double total = 0.0;
	for (const auto& effect : result.effects)
	{
		total += effect.value.y;
	}
	return total;
}

bool same_position(
	const Common::Vec3& left,
	const Common::Vec3& right)
{
	return std::fabs(left.x - right.x) <= kTolerance &&
		std::fabs(left.y - right.y) <= kTolerance &&
		std::fabs(left.z - right.z) <= kTolerance;
}

bool has_effect_at_position(
	const Core::Simulation::GroundInteractionResult& result,
	const Common::Vec3& position)
{
	for (const auto& effect : result.effects)
	{
		if (same_position(effect.position, position))
		{
			return true;
		}
	}
	return false;
}

Data::GroundInteractionDefinition
	make_enabled_ground_definition()
{
	Data::AircraftConfig config = Tests::Fck1c::make_test_config();
	config.suspension.enable_fallback_ground_forces = true;
	return Data::make_ground_interaction_definition(
		config.suspension);
}

struct GroundModelFixture
{
	GroundModelFixture()
		: definition(make_enabled_ground_definition()),
		model(definition)
	{
		gear.position = 1.0;
		observation.altitude_agl = 2.25;
		observation.current_mass = 10000.0;
		observation.velocity_body.x = 10.0;
	}

	const Core::Simulation::GroundInteractionResult& step(
		const Core::FrameDataAvailability& availability)
	{
		return model.step({
			engines,
			gear,
			observation,
			availability,
			propulsion
		});
	}

	Data::GroundInteractionDefinition definition;
	Core::Simulation::GroundInteractionModel model;
	Core::EngineData engines;
	Core::LandingGearData gear;
	Core::AircraftState observation;
	Core::Simulation::PropulsionResult propulsion;
};

Core::AircraftState make_aerodynamic_observation()
{
	Core::AircraftState observation;
	observation.atmosphere_density = 1.225;
	observation.speed_scalar = 100.0;
	observation.mach = 0.0;
	observation.alpha = 5.0;
	observation.roll_rate = 0.2;
	observation.yaw_rate = 0.1;
	return observation;
}

void test_aerodynamics_model_effect_groups(Tests::Context& context)
{
	const Data::AircraftConfig config = Tests::Fck1c::make_test_config();
	Core::Simulation::AerodynamicsModel model(config.aerodynamics);
	const Core::PrimaryControlPosition primary;
	const Core::SecondaryControlPosition secondary;
	const Core::LandingGearData landing_gear;
	const Core::AirframeIntegrity integrity;
	const Core::AircraftState observation = make_aerodynamic_observation();
	const auto& normal = model.step({
		primary, secondary, landing_gear, integrity, observation, false });
	TEST_EXPECT(
		context,
		normal.primary_effects.size() == kPrimaryAerodynamicEffectCount);
	TEST_EXPECT(
		context,
		normal.limiter_effects.size() == kNormalLimiterEffectCount);
	const auto& easy = model.step({
		primary, secondary, landing_gear, integrity, observation, true });
	TEST_EXPECT(
		context,
		easy.limiter_effects.size() == kEasyFlightLimiterEffectCount);
}

Core::Simulation::PropulsionResult run_propulsion(
	const Data::AircraftConfig& config,
	const Core::EngineData& engines,
	const Core::MaxPowerCommand& max_power = {})
{
	Core::Simulation::PropulsionModel model({
		config.engine.mach_table,
		config.engine.max_thrust_table,
		config.engine.afterburner.thrust_factor,
		config.left_engine_position,
		config.right_engine_position
	});
	Core::AircraftState observation;
	observation.engine_alt_effect = 1.0;
	return model.step({ engines, observation, max_power });
}

void test_propulsion_operating_points(Tests::Context& context)
{
	const Data::AircraftConfig config = Tests::Fck1c::make_test_config();
	Core::EngineData engines;
	const auto idle = run_propulsion(config, engines);
	TEST_EXPECT_NEAR(context, idle.left_thrust_force, 0.0, kTolerance);
	engines.left.throttle_output = kFullThrottle;
	engines.right.throttle_output = kFullThrottle;
	const auto dry = run_propulsion(config, engines);
	TEST_EXPECT_NEAR(
		context,
		dry.left_thrust_force,
		kExpectedFullDryThrustPerEngine,
		kTolerance);
	engines.left.afterburner_ratio = 1.0;
	engines.right.afterburner_ratio = 1.0;
	const auto afterburner = run_propulsion(config, engines);
	TEST_EXPECT(
		context,
		afterburner.left_thrust_force > dry.left_thrust_force);
	const auto cut = run_propulsion(config, engines, { 1.0, 0.0 });
	TEST_EXPECT_NEAR(context, cut.left_thrust_force, 0.0, kTolerance);
}

void test_propulsion_applies_engine_condition(Tests::Context& context)
{
	const Data::AircraftConfig config = Tests::Fck1c::make_test_config();
	Core::EngineData engines;
	engines.left.throttle_output = kFullThrottle;
	engines.right.throttle_output = kFullThrottle;
	engines.left.condition = kDamagedCondition;
	const auto result = run_propulsion(config, engines);
	TEST_EXPECT_NEAR(
		context, result.left_thrust_force, kExpectedDamagedThrust, kTolerance);
	TEST_EXPECT_NEAR(
		context,
		result.right_thrust_force,
		kExpectedFullDryThrustPerEngine,
		kTolerance);
}

void test_ground_model_selects_one_force_source(Tests::Context& context)
{
	GroundModelFixture fixture;
	const Core::FrameDataAvailability unavailable;
	const auto& fallback = fixture.step(unavailable);
	TEST_EXPECT(context, fallback.used_fallback);
	TEST_EXPECT(
		context,
		fallback.effects.size() == kFullFallbackEffectCount);
	TEST_EXPECT_NEAR(
		context,
		total_vertical_force(fallback),
		kExpectedFullFallbackVerticalForce,
		kTolerance);
	Core::FrameDataAvailability feedback;
	feedback.suspension.fill(true);
	const auto& dcs = fixture.step(feedback);
	TEST_EXPECT(context, !dcs.used_fallback);
	TEST_EXPECT(context, dcs.effects.empty());
	Core::FrameDataAvailability partial_feedback;
	partial_feedback.suspension[kLeftMainWheelIndex] = true;
	const auto& partial = fixture.step(partial_feedback);
	TEST_EXPECT(context, partial.used_fallback);
	TEST_EXPECT(
		context,
		partial.effects.size() == kPartialFallbackEffectCount);
	TEST_EXPECT_NEAR(
		context,
		total_vertical_force(partial),
		kExpectedPartialFallbackVerticalForce,
		kTolerance);
	TEST_EXPECT(
		context,
		!has_effect_at_position(
			partial,
			fixture.definition.gear_points[kLeftMainWheelIndex]));
}

void test_ground_model_ignores_stale_feedback(
	Tests::Context& context)
{
	GroundModelFixture fixture;
	fixture.gear.position = 0.0;
	fixture.gear.any_weight_on_wheels = true;
	fixture.gear.suspension[kLeftMainWheelIndex].weight_on_wheel =
		true;
	fixture.observation.altitude_agl = kBellyContactAltitude;
	Core::FrameDataAvailability current_feedback;
	current_feedback.suspension.fill(true);
	TEST_EXPECT(
		context,
		!fixture.step(current_feedback).used_fallback);
	const Core::FrameDataAvailability unavailable;
	const auto& fallback = fixture.step(unavailable);
	TEST_EXPECT(context, fallback.used_fallback);
	TEST_EXPECT(
		context,
		fallback.effects.size() == kBellyFallbackEffectCount);
	TEST_EXPECT(
		context,
		has_effect_at_position(
			fallback,
			fixture.definition.belly_point));
}
}

void run_simulation_model_tests(Tests::Context& context)
{
	test_aerodynamics_model_effect_groups(context);
	test_propulsion_operating_points(context);
	test_propulsion_applies_engine_condition(context);
	test_ground_model_selects_one_force_source(context);
	test_ground_model_ignores_stale_feedback(context);
}
