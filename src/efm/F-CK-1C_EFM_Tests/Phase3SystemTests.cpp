#include "SystemPipelineTestFixture.h"

#include "Common/Units.h"
#include "Core/Systems/FlightControlComputer/FlightControlComputer.h"

#include <array>

namespace
{
using namespace Core;
using namespace Core::Systems;
using namespace SystemPipelineTest;

constexpr double kTolerance = 1e-12;
constexpr double kPitchInput = 0.25;
constexpr double kYawInput = 0.4;
constexpr double kDamagedEngineIntegrity = 0.2;
constexpr double kDamagedWingIntegrity = 0.35;
constexpr double kFullIntegrity = 1.0;
constexpr double kFailedIntegrity = 0.0;
constexpr double kFullBrakeInput = 1.0;
constexpr double kFrameDt = 0.02;
constexpr double kAltitudeAsl = 1000.0;
constexpr double kSurfaceHeight = 200.0;
constexpr double kAltitudeAgl = 800.0;
constexpr double kUpdatedAltitudeAsl = 1500.0;
constexpr double kDensity = 1.2;
constexpr double kSpeedOfSound = 200.0;
constexpr double kAtmosphereTemperature = 288.0;
constexpr double kAtmospherePressure = 101325.0;
constexpr double kForwardSpeed = 100.0;
constexpr double kMach = 0.5;
constexpr double kDynamicPressure = 6000.0;
constexpr double kBodyAccelerationY = 9.81;
constexpr double kExpectedGLoad = 2.0;
constexpr double kAngleOfAttack = 0.1;
constexpr double kAngleOfSlide = -0.05;
constexpr std::size_t kExpectedRepairSubscribers = 3;
constexpr std::size_t kDamageAreaCount = 6;

struct LandingGearDamageCase
{
	LandingGearDamageSegment segment;
	bool nose_failed = false;
	bool left_main_failed = false;
	bool right_main_failed = false;
};

AtmosphereInput valid_atmosphere(double altitude_asl)
{
	AtmosphereInput input;
	input.altitude_asl = altitude_asl;
	input.temperature = kAtmosphereTemperature;
	input.speed_of_sound = kSpeedOfSound;
	input.density = kDensity;
	input.pressure = kAtmospherePressure;
	return input;
}

void expect_handled(
	Tests::Context& context,
	SystemPipeline& pipeline,
	const Command& command)
{
	TEST_EXPECT(
		context,
		pipeline.send(command) == DispatchResult::Handled);
}

void test_owner_handlers_are_registered(Tests::Context& context)
{
	SystemPipeline pipeline(flight_setup());
	const std::array<Command, 4> commands = {{
		{ CommandId::SetPitchAxis, kPitchInput },
		{ CommandId::SetLeftEngine, kFullIntegrity },
		{ CommandId::SetFlapsDown, kFullIntegrity },
		{ CommandId::SetGear, kFullIntegrity }
	}};
	for (const Command& command : commands)
	{
		expect_handled(context, pipeline, command);
	}
	TEST_EXPECT(
		context,
		pipeline.send({}) == DispatchResult::Unhandled);

	const std::array<DamageArea, kDamageAreaCount> areas = {{
		DamageArea::LeftWing,
		DamageArea::RightWing,
		DamageArea::Tail,
		DamageArea::LeftEngine,
		DamageArea::RightEngine,
		DamageArea::LandingGear
	}};
	for (DamageArea area : areas)
	{
		TEST_EXPECT(
			context,
			pipeline.apply({ area, 0, kFullIntegrity }) ==
				DispatchResult::Handled);
	}
}

void test_control_data_crosses_owner_boundary(Tests::Context& context)
{
	SystemPipeline pipeline(flight_setup());
	expect_handled(
		context,
		pipeline,
		{ CommandId::SetPitchAxis, kPitchInput });
	expect_handled(
		context,
		pipeline,
		{ CommandId::SetYawAxis, kYawInput });
	FrameInput input;
	input.dt_s = kFrameDt;
	const AircraftObservation observation;
	const AircraftDataSnapshot output =
		step_pipeline(pipeline, input, observation);
	const PilotControlState& pilot =
		output.read(AircraftDataKeys::kPilotControlState);
	const FlightControlDemand& demand =
		output.read(AircraftDataKeys::kFlightControlDemand);
	const PrimaryControlPosition& position =
		output.read(AircraftDataKeys::kPrimaryControlPosition);
	TEST_EXPECT_NEAR(context, pilot.pitch, kPitchInput, kTolerance);
	TEST_EXPECT_NEAR(context, pilot.yaw, -kYawInput, kTolerance);
	TEST_EXPECT_NEAR(
		context, position.elevator, demand.pitch, kTolerance);
	TEST_EXPECT_NEAR(
		context, position.aileron, demand.roll, kTolerance);
	TEST_EXPECT_NEAR(
		context, position.rudder, demand.yaw, kTolerance);
}

FrameInput nonzero_observation_frame()
{
	FrameInput input;
	input.dt_s = kFrameDt;
	input.availability.atmosphere = true;
	input.availability.surface = true;
	input.availability.world_kinematics = true;
	input.availability.body_kinematics = true;
	input.atmosphere = valid_atmosphere(kAltitudeAsl);
	input.surface.surface_height = kSurfaceHeight;
	input.world_kinematics.velocity.x = kForwardSpeed;
	input.body_kinematics.acceleration.y = kBodyAccelerationY;
	input.body_kinematics.angle_of_attack = kAngleOfAttack;
	input.body_kinematics.angle_of_slide = kAngleOfSlide;
	return input;
}

FlightControlDemand expected_nonzero_demand()
{
	FlightControlComputer reference(
		fck1c_flight_control_computer_config(),
		StartMode::HotGround);
	::Systems::FBWControllerInput input;
	input.dt = kFrameDt;
	input.qbar = kDynamicPressure;
	input.alpha = Common::deg(kAngleOfAttack);
	input.beta = Common::deg(kAngleOfSlide);
	input.speed_scalar = kForwardSpeed;
	input.mach = kMach;
	input.g = kExpectedGLoad;
	input.gear_pos = kFullIntegrity;
	return reference.step(input, {});
}

void expect_normalized_observation(
	Tests::Context& context,
	const AircraftDataSnapshot& output)
{
	const AircraftObservation& observation =
		output.read(AircraftDataKeys::kAircraftObservation);
	TEST_EXPECT_NEAR(
		context, observation.altitude_agl, kAltitudeAgl, kTolerance);
	TEST_EXPECT_NEAR(
		context, observation.speed_scalar, kForwardSpeed, kTolerance);
	TEST_EXPECT_NEAR(
		context, observation.ground_speed, kForwardSpeed, kTolerance);
	TEST_EXPECT_NEAR(context, observation.mach, kMach, kTolerance);
	TEST_EXPECT_NEAR(
		context, observation.dynamic_pressure, kDynamicPressure, kTolerance);
	TEST_EXPECT_NEAR(
		context, observation.g_load, kExpectedGLoad, kTolerance);
	TEST_EXPECT_NEAR(
		context,
		observation.alpha_deg,
		Common::deg(kAngleOfAttack),
		kTolerance);
	TEST_EXPECT_NEAR(
		context,
		observation.beta_deg,
		Common::deg(kAngleOfSlide),
		kTolerance);
}

void test_observations_are_normalized_and_retained(Tests::Context& context)
{
	SystemPipeline pipeline(flight_setup());
	AircraftState observation_state;
	const FrameInput first_frame = nonzero_observation_frame();
	apply_aircraft_observations(observation_state, first_frame);
	update_airspeed(observation_state);
	const AircraftDataSnapshot first =
		step_pipeline(
			pipeline,
			first_frame,
			make_aircraft_observation(observation_state));
	expect_normalized_observation(context, first);
	const FlightControlDemand expected = expected_nonzero_demand();
	const FlightControlDemand& actual =
		first.read(AircraftDataKeys::kFlightControlDemand);
	TEST_EXPECT_NEAR(context, actual.pitch, expected.pitch, kTolerance);
	TEST_EXPECT_NEAR(context, actual.roll, expected.roll, kTolerance);
	TEST_EXPECT_NEAR(context, actual.yaw, expected.yaw, kTolerance);
	for (double spin :
		first.read(AircraftDataKeys::kLandingGearData).wheel_spin)
	{
		TEST_EXPECT_NEAR(context, spin, kNeutralAxis, kTolerance);
	}
	FrameInput no_new_observation;
	no_new_observation.dt_s = kFrameDt;
	expect_normalized_observation(
		context,
		step_pipeline(
			pipeline,
			no_new_observation,
			make_aircraft_observation(observation_state)));
	FrameInput atmosphere_only;
	atmosphere_only.dt_s = kFrameDt;
	atmosphere_only.availability.atmosphere = true;
	atmosphere_only.atmosphere = valid_atmosphere(kUpdatedAltitudeAsl);
	apply_aircraft_observations(observation_state, atmosphere_only);
	update_airspeed(observation_state);
	const AircraftObservation& updated =
		step_pipeline(
			pipeline,
			atmosphere_only,
			make_aircraft_observation(observation_state))
			.read(AircraftDataKeys::kAircraftObservation);
	TEST_EXPECT_NEAR(
		context, updated.altitude_asl, kUpdatedAltitudeAsl, kTolerance);
	TEST_EXPECT_NEAR(
		context, updated.altitude_agl, kAltitudeAgl, kTolerance);
}

void test_damage_and_repair_reach_semantic_owners(Tests::Context& context)
{
	SystemPipeline pipeline(flight_setup());
	TEST_EXPECT(
		context,
		pipeline.apply({
			DamageArea::LeftEngine, 0, kDamagedEngineIntegrity
		}) == DispatchResult::Handled);
	TEST_EXPECT(
		context,
		pipeline.apply({
			DamageArea::LeftWing, 0, kDamagedWingIntegrity
		}) == DispatchResult::Handled);
	const AircraftDataSnapshot damaged = step_pipeline(pipeline);
	TEST_EXPECT_NEAR(
		context,
		damaged.read(AircraftDataKeys::kEngineData).left.condition,
		kDamagedEngineIntegrity,
		kTolerance);
	TEST_EXPECT_NEAR(
		context,
		damaged.read(AircraftDataKeys::kAirframeIntegrity).left_wing,
		kDamagedWingIntegrity,
		kTolerance);

	TEST_EXPECT(
		context,
		pipeline.apply(RepairEvent{}) == kExpectedRepairSubscribers);
	const AircraftDataSnapshot repaired = step_pipeline(pipeline);
	TEST_EXPECT_NEAR(
		context,
		repaired.read(AircraftDataKeys::kEngineData).left.condition,
		kFullIntegrity,
		kTolerance);
	TEST_EXPECT_NEAR(
		context,
		repaired.read(AircraftDataKeys::kAirframeIntegrity).left_wing,
		kFullIntegrity,
		kTolerance);
}

void configure_landing_gear_controls(
	Tests::Context& context,
	SystemPipeline& pipeline)
{
	expect_handled(context, pipeline, {
		CommandId::SetYawAxis, kYawInput
	});
	expect_handled(context, pipeline, {
		CommandId::SetLeftBrake, kFullBrakeInput
	});
	expect_handled(context, pipeline, {
		CommandId::SetRightBrake, kFullBrakeInput
	});
}

void expect_other_damage_owners_healthy(
	Tests::Context& context,
	const AircraftDataSnapshot& snapshot)
{
	const EngineData& engine = snapshot.read(AircraftDataKeys::kEngineData);
	const AirframeIntegrity& airframe =
		snapshot.read(AircraftDataKeys::kAirframeIntegrity);
	TEST_EXPECT_NEAR(
		context, engine.left.condition, kFullIntegrity, kTolerance);
	TEST_EXPECT_NEAR(
		context, engine.right.condition, kFullIntegrity, kTolerance);
	TEST_EXPECT_NEAR(
		context, airframe.left_wing, kFullIntegrity, kTolerance);
	TEST_EXPECT_NEAR(
		context, airframe.right_wing, kFullIntegrity, kTolerance);
	TEST_EXPECT_NEAR(
		context, airframe.tail, kFullIntegrity, kTolerance);
}

void expect_landing_gear_damage_case(
	Tests::Context& context,
	const AircraftDataSnapshot& snapshot,
	const LandingGearDamageCase& expected)
{
	const LandingGearData& gear =
		snapshot.read(AircraftDataKeys::kLandingGearData);
	if (expected.nose_failed)
	{
		TEST_EXPECT_NEAR(
			context, gear.nose_wheel_steering, 0.0, kTolerance);
	}
	else
	{
		TEST_EXPECT(context, gear.nose_wheel_steering != 0.0);
	}
	TEST_EXPECT_NEAR(
		context,
		gear.brake_left,
		expected.left_main_failed ? 0.0 : kFullBrakeInput,
		kTolerance);
	TEST_EXPECT_NEAR(
		context,
		gear.brake_right,
		expected.right_main_failed ? 0.0 : kFullBrakeInput,
		kTolerance);
	expect_other_damage_owners_healthy(context, snapshot);
}

void test_landing_gear_damage_is_routed_and_isolated(
	Tests::Context& context)
{
	SystemPipeline pipeline(flight_setup());
	configure_landing_gear_controls(context, pipeline);
	const std::array<
		LandingGearDamageCase,
		kLandingGearDamageSegmentCount> cases = {{
		{ LandingGearDamageSegment::Nose, true, false, false },
		{ LandingGearDamageSegment::LeftMain, false, true, false },
		{ LandingGearDamageSegment::RightMain, false, false, true }
	}};
	for (const LandingGearDamageCase& test_case : cases)
	{
		TEST_EXPECT(
			context,
			pipeline.apply({
				DamageArea::LandingGear,
				landing_gear_segment_index(test_case.segment),
				kFailedIntegrity
			}) == DispatchResult::Handled);
		expect_landing_gear_damage_case(
			context, step_pipeline(pipeline), test_case);
		TEST_EXPECT(
			context,
			pipeline.apply(RepairEvent{}) == kExpectedRepairSubscribers);
		expect_landing_gear_damage_case(
			context,
			step_pipeline(pipeline),
			{ test_case.segment, false, false, false });
	}
}
}

void run_phase_three_system_tests(Tests::Context& context)
{
	test_owner_handlers_are_registered(context);
	test_control_data_crosses_owner_boundary(context);
	test_observations_are_normalized_and_retained(context);
	test_damage_and_repair_reach_semantic_owners(context);
	test_landing_gear_damage_is_routed_and_isolated(context);
}
