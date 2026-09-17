#include "Fck1cEfmTestFixture.h"
#include "SystemPipelineTestFixture.h"
#include "TestHarness.h"

#include "Core/Diagnostics/ExecutionError.h"
#include "Core/Simulation/AircraftState.h"
#include "Core/Simulation/Models/ModelExecutionContext.h"
#include "Core/Simulation/Models/Aerodynamics/AerodynamicsModel.h"
#include "Core/Simulation/Models/GroundInteraction/GroundInteractionModel.h"
#include "Core/Simulation/Models/Propulsion/PropulsionModel.h"
#include "Core/Simulation/SimulationPipeline.h"
#include "DcsBridge/Internal/FrameInputCollector.h"

#include <cmath>
#include <limits>
#include <stdexcept>

namespace
{
constexpr double kTolerance = 1e-9;
constexpr std::size_t kPrimaryAerodynamicEffectCount = 7;
constexpr std::size_t kNormalSupplementalEffectCount = 1;
constexpr std::size_t kEasyFlightSupplementalEffectCount = 4;
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
constexpr double kSimulationStepSeconds = 0.01;

void test_aircraft_state_default_density_uses_si(Tests::Context& context)
{
	const Core::AircraftState state;
	TEST_EXPECT_NEAR(
		context,
		state.atmosphere_density_kg_m3,
		Core::kSeaLevelAirDensityKgM3,
		kTolerance);
}

double total_vertical_force(
	const Core::Simulation::GroundInteractionResult& result)
{
	double total = 0.0;
	for (const auto& effect : result.effects)
	{
		total += effect.force_body_n.y;
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
		if (same_position(effect.application_position_body_m, position))
		{
			return true;
		}
	}
	return false;
}

Core::Simulation::GroundInteractionConfig make_enabled_ground_config()
{
	auto config = Core::Simulation::fck1c_ground_interaction_config();
	config.enable_fallback_ground_forces = true;
	return config;
}

struct GroundModelFixture
{
	GroundModelFixture()
		: config(make_enabled_ground_config()),
		model(config)
	{
		gear.position_normalized = 1.0;
		gear.wheel_radius_m =
			Tests::Fck1c::make_test_config().landing_gear.wheel_radius_m;
		observation.altitude_agl_m = 2.25;
		observation.mass_kg = 10000.0;
		observation.velocity_body_mps.x = 10.0;
	}

	const Core::Simulation::GroundInteractionResult& step(
		const Core::FrameDataAvailability& availability)
	{
		return model.step({
			engines,
			gear,
			observation,
			availability,
			total_thrust_force_n
		});
	}

	Core::Simulation::GroundInteractionConfig config;
	Core::Simulation::GroundInteractionModel model;
	Core::EngineData engines;
	Core::LandingGearData gear;
	Core::AircraftState observation;
	double total_thrust_force_n = 0.0;
};

Core::AircraftState make_aerodynamic_observation()
{
	Core::AircraftState observation;
	observation.atmosphere_density_kg_m3 = 1.225;
	observation.true_airspeed_mps = 100.0;
	observation.mach = 0.0;
	observation.angle_of_attack_deg = 5.0;
	observation.roll_rate_rad_s = 0.2;
	observation.yaw_rate_rad_s = 0.1;
	return observation;
}

void test_aerodynamics_model_effect_groups(Tests::Context& context)
{
	const auto config = Tests::Fck1c::make_test_config();
	Core::Simulation::AerodynamicsModel model(config.aerodynamics);
	const Core::FlightControlActuatorState primary;
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
		normal.supplemental_effects.size() ==
			kNormalSupplementalEffectCount);
	const auto& easy = model.step({
		primary, secondary, landing_gear, integrity, observation, true });
	TEST_EXPECT(
		context,
		easy.supplemental_effects.size() ==
			kEasyFlightSupplementalEffectCount);
}

Core::Simulation::PropulsionResult run_propulsion(
	const Core::Simulation::PropulsionConfig& config,
	const Core::EngineData& engines,
	const Core::PropulsionTestIntent& diagnostics = {})
{
	Core::Simulation::PropulsionModel model(config);
	Core::AircraftState observation;
	observation.engine_altitude_factor = 1.0;
	return model.step({ engines, observation, diagnostics });
}

void test_propulsion_operating_points(Tests::Context& context)
{
	const auto config = Tests::Fck1c::make_test_config();
	Core::EngineData engines;
	const auto idle = run_propulsion(config.propulsion, engines);
	TEST_EXPECT_NEAR(context, idle.left_thrust_force_n, 0.0, kTolerance);
	engines.left.throttle_output_normalized = kFullThrottle;
	engines.right.throttle_output_normalized = kFullThrottle;
	const auto dry = run_propulsion(config.propulsion, engines);
	TEST_EXPECT_NEAR(
		context,
		dry.left_thrust_force_n,
		kExpectedFullDryThrustPerEngine,
		kTolerance);
	engines.left.afterburner_ratio_0_1 = 1.0;
	engines.right.afterburner_ratio_0_1 = 1.0;
	const auto afterburner = run_propulsion(config.propulsion, engines);
	TEST_EXPECT(
		context,
		afterburner.left_thrust_force_n > dry.left_thrust_force_n);
	const auto cut = run_propulsion(
		config.propulsion, engines, { true });
	TEST_EXPECT_NEAR(context, cut.left_thrust_force_n, 0.0, kTolerance);
}

void test_propulsion_applies_engine_condition(Tests::Context& context)
{
	const auto config = Tests::Fck1c::make_test_config();
	Core::EngineData engines;
	engines.left.throttle_output_normalized = kFullThrottle;
	engines.right.throttle_output_normalized = kFullThrottle;
	engines.left.condition_0_1 = kDamagedCondition;
	const auto result = run_propulsion(config.propulsion, engines);
	TEST_EXPECT_NEAR(
		context, result.left_thrust_force_n, kExpectedDamagedThrust, kTolerance);
	TEST_EXPECT_NEAR(
		context,
		result.right_thrust_force_n,
		kExpectedFullDryThrustPerEngine,
		kTolerance);
}

SystemPipelineTest::SystemDefinition invalid_aerodynamics_state()
{
	using namespace Core;
	using namespace Core::Systems;
	return {
		"test_state",
		SystemGroup::Equipment,
		[](SystemSetup& setup)
		{
			FlightControlActuatorState primary;
			primary.symmetric_stabilator.position_rad =
				std::numeric_limits<double>::quiet_NaN();
			setup.publish(
				AircraftDataKeys::kFlightControlActuatorState,
				primary);
			setup.publish(
				AircraftDataKeys::kSecondaryControlPosition,
				SecondaryControlPosition{});
			setup.publish(AircraftDataKeys::kLandingGearData, LandingGearData{});
			setup.publish(
				AircraftDataKeys::kAirframeIntegrity, AirframeIntegrity{});
			setup.publish(AircraftDataKeys::kEngineData, EngineData{});
			setup.publish(AircraftDataKeys::kFuelData, FuelData{});
			setup.publish(
				AircraftDataKeys::kPropulsionTestIntent,
				PropulsionTestIntent{});
		},
		SystemPipelineTest::no_step()
	};
}

void test_model_error_identifies_runtime_owner(Tests::Context& context)
{
	using namespace Core;
	using namespace Core::Systems;
	using namespace SystemPipelineTest;
	SystemPipeline aircraft(
		flight_setup(),
		{ entry(invalid_aerodynamics_state()) });
	Simulation::SimulationPipeline simulation(
		Tests::Fck1c::make_test_models(Tests::Fck1c::make_test_config()));
	const AircraftDataSnapshot snapshot = aircraft.snapshot();
	const AircraftState observation;
	const FrameInput frame;
	expect_execution_error(
		context,
		[&]() { (void)simulation.step({ snapshot, observation, frame }); },
		{
			ExecutionOwnerType::SimulationModel,
			"aerodynamics",
			"step",
			"Aerodynamics input requires finite primary surface positions."
		});
}

void test_model_create_error_identifies_runtime_owner(
	Tests::Context& context)
{
	using namespace Core;
	using namespace Core::Simulation;
	using namespace SystemPipelineTest;
	expect_execution_error(
		context,
		[]()
		{
			(void)Detail::invoke_model_action(
				Detail::kAerodynamicsOwner,
				Detail::kModelCreateOperation,
				[]() -> int
				{
					throw std::runtime_error(
						"expected model construction failure");
				});
		},
		{
			ExecutionOwnerType::SimulationModel,
			"aerodynamics",
			"create",
			"expected model construction failure"
		});
}

void test_execution_error_is_not_rewrapped(Tests::Context& context)
{
	using namespace Core;
	using namespace Core::Simulation;
	using namespace SystemPipelineTest;
	expect_execution_error(
		context,
		[]()
		{
			(void)Detail::invoke_model_action(
				Detail::kAerodynamicsOwner,
				Detail::kModelCreateOperation,
				[]() -> int
				{
					throw ExecutionError({
						ExecutionOwnerType::System,
						"inner_system",
						"setup",
						"original failure"
					});
				});
		},
		{
			ExecutionOwnerType::System,
			"inner_system",
			"setup",
			"original failure"
		});
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
			fixture.config.gear_points_body_m[kLeftMainWheelIndex]));
}

void test_ground_model_ignores_stale_feedback(
	Tests::Context& context)
{
	GroundModelFixture fixture;
	fixture.gear.position_normalized = 0.0;
	fixture.gear.any_weight_on_wheels = true;
	fixture.gear.suspension[kLeftMainWheelIndex].weight_on_wheel =
		true;
	fixture.observation.altitude_agl_m = kBellyContactAltitude;
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
			fixture.config.belly_point_body_m));
}

void test_ground_model_uses_collector_frame_freshness(
	Tests::Context& context)
{
	DcsBridge::Internal::FrameInputCollector collector;
	for (int index = 0;
		index < static_cast<int>(Core::kFrameSuspensionWheelCount);
		++index)
	{
		Core::SuspensionFeedbackInput sample;
		sample.index = index;
		TEST_EXPECT(context, collector.publish_suspension(sample));
	}
	GroundModelFixture fixture;
	const Core::FrameInput feedback_frame =
		collector.snapshot(kSimulationStepSeconds);
	TEST_EXPECT(
		context,
		!fixture.step(feedback_frame.availability).used_fallback);
	const Core::FrameInput missing_frame =
		collector.snapshot(kSimulationStepSeconds);
	TEST_EXPECT(
		context,
		fixture.step(missing_frame.availability).used_fallback);
}
}

void run_simulation_model_tests(Tests::Context& context)
{
	test_aircraft_state_default_density_uses_si(context);
	test_aerodynamics_model_effect_groups(context);
	test_propulsion_operating_points(context);
	test_propulsion_applies_engine_condition(context);
	test_model_error_identifies_runtime_owner(context);
	test_model_create_error_identifies_runtime_owner(context);
	test_execution_error_is_not_rewrapped(context);
	test_ground_model_selects_one_force_source(context);
	test_ground_model_ignores_stale_feedback(context);
	test_ground_model_uses_collector_frame_freshness(context);
}
