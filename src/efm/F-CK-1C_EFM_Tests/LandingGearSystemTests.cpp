#include "TestHarness.h"

#include "Core/Systems/LandingGear/LandingGear.h"
#include "Core/Systems/LandingGear/LandingGearModel.h"

namespace
{
constexpr double kTolerance = 1e-9;
constexpr double kFullIntegrity = 1.0;
constexpr double kNoseIntegrity = 0.5;
constexpr double kLeftMainIntegrity = 0.25;
constexpr double kRightMainIntegrity = 0.75;
constexpr double kYawInput = 0.4;
constexpr double kBrakeInput = 1.0;
constexpr double kFrameDt = 0.02;

void test_brake_axis_normalization(Tests::Context& context)
{
	TEST_EXPECT_NEAR(context, Systems::normalize_brake_axis(-1.0), 0.0, kTolerance);
	TEST_EXPECT_NEAR(context, Systems::normalize_brake_axis(0.5), 0.5, kTolerance);
}

void test_gear_actuator(Tests::Context& context)
{
	Systems::LandingGearSystemState landing_gear;
	Systems::set_gear(landing_gear, true);
	Systems::update_gear_position(landing_gear);
	TEST_EXPECT_NEAR(context, landing_gear.position, 0.001, kTolerance);

	Systems::set_gear(landing_gear, false);
	Systems::update_gear_position(landing_gear);
	TEST_EXPECT_NEAR(context, landing_gear.position, 0.0, kTolerance);
}

void test_start_configuration(Tests::Context& context)
{
	Systems::LandingGearSystemState landing_gear;
	Systems::configure_ground_start_landing_gear(landing_gear);
	TEST_EXPECT(context, landing_gear.switch_down);
	TEST_EXPECT_NEAR(context, landing_gear.position, 1.0, kTolerance);
	TEST_EXPECT(context, landing_gear.wheels.nose_turn_enabled);

	Systems::configure_air_start_landing_gear(landing_gear);
	TEST_EXPECT(context, !landing_gear.switch_down);
	TEST_EXPECT_NEAR(context, landing_gear.position, 0.0, kTolerance);
	TEST_EXPECT(context, !landing_gear.wheels.nose_turn_enabled);
}

void test_damage_scales_owned_equipment_and_repair_restores_it(
	Tests::Context& context)
{
	Core::Systems::LandingGear landing_gear(
		Core::StartMode::HotGround,
		Core::Systems::fck1c_landing_gear_config());
	landing_gear.handle_command({
		Core::CommandGroup::LandingGear,
		Core::CommandId::SetLeftBrake,
		kBrakeInput
	});
	landing_gear.handle_command({
		Core::CommandGroup::LandingGear,
		Core::CommandId::SetRightBrake,
		kBrakeInput
	});
	const Core::Systems::LandingGearFrameInput input = {
		0.0, 0.0, kFrameDt, 0.0, kYawInput
	};
	const Core::LandingGearData healthy = landing_gear.step(input);
	TEST_EXPECT(context, healthy.nose_wheel_steering != 0.0);

	landing_gear.apply_damage({
		Core::DamageArea::LandingGear,
		Core::landing_gear_segment_index(
			Core::LandingGearDamageSegment::Nose),
		kNoseIntegrity
	});
	landing_gear.apply_damage({
		Core::DamageArea::LandingGear,
		Core::landing_gear_segment_index(
			Core::LandingGearDamageSegment::LeftMain),
		kLeftMainIntegrity
	});
	landing_gear.apply_damage({
		Core::DamageArea::LandingGear,
		Core::landing_gear_segment_index(
			Core::LandingGearDamageSegment::RightMain),
		kRightMainIntegrity
	});
	const Core::LandingGearData damaged = landing_gear.data();
	TEST_EXPECT_NEAR(
		context,
		damaged.nose_wheel_steering,
		healthy.nose_wheel_steering * kNoseIntegrity,
		kTolerance);
	TEST_EXPECT_NEAR(
		context,
		damaged.brake_left,
		healthy.brake_left * kLeftMainIntegrity,
		kTolerance);
	TEST_EXPECT_NEAR(
		context,
		damaged.brake_right,
		healthy.brake_right * kRightMainIntegrity,
		kTolerance);

	landing_gear.repair({});
	const Core::LandingGearData repaired = landing_gear.data();
	TEST_EXPECT_NEAR(
		context,
		repaired.nose_wheel_steering,
		healthy.nose_wheel_steering,
		kTolerance);
	TEST_EXPECT_NEAR(
		context, repaired.brake_left, healthy.brake_left, kTolerance);
	TEST_EXPECT_NEAR(
		context, repaired.brake_right, healthy.brake_right, kTolerance);
}
}

void run_landing_gear_system_tests(Tests::Context& context)
{
	test_brake_axis_normalization(context);
	test_gear_actuator(context);
	test_start_configuration(context);
	test_damage_scales_owned_equipment_and_repair_restores_it(context);
}
