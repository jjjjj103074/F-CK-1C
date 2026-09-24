#include "TestHarness.h"

#include "Common/Units.h"
#include "Core/Systems/FlightControlComputer/ModeAndGainScheduling/ModeAndGainScheduling.h"

#include <stdexcept>

namespace
{
constexpr double kTolerance = 1e-9;
Systems::ManeuverEnvelope make_cat1_envelope(
	const Systems::ModeAndGainSchedulingConfig& config,
	bool override_active = false)
{
	return Systems::make_maneuver_envelope(
		{ config, config.cat1, 0.0, override_active, 2.0 });
}

void test_reference_guidance_envelope(Tests::Context& context)
{
	const auto config =
		Systems::make_fck1c_mode_and_gain_scheduling_config();
	const auto envelope = make_cat1_envelope(config);
	TEST_EXPECT_NEAR(
		context, envelope.guidance.bank_limit_rad, Common::rad(30.0), kTolerance);
	TEST_EXPECT_NEAR(
		context, envelope.guidance.roll_rate_limit_rad_s,
		Common::rad(20.0), kTolerance);
	TEST_EXPECT_NEAR(
		context, envelope.guidance.minimum_normal_acceleration_g, 0.5, kTolerance);
	TEST_EXPECT_NEAR(
		context, envelope.guidance.maximum_normal_acceleration_g, 2.0, kTolerance);
}

void test_guidance_must_fit_inside_hard_protection(Tests::Context& context)
{
	auto config = Systems::make_fck1c_mode_and_gain_scheduling_config();
	config.guidance_roll_rate_limit_rad_s =
		config.cat3.envelope.roll_rate_limit_rad_s + 0.1;
	bool rejected = false;
	try
	{
		(void)Systems::make_maneuver_envelope(
			{ config, config.cat3, 0.0, false, 0.0 });
	}
	catch (const std::invalid_argument&)
	{
		rejected = true;
	}
	TEST_EXPECT(context, rejected);
}

void test_developer_override_is_explicit(Tests::Context& context)
{
	const auto config =
		Systems::make_fck1c_mode_and_gain_scheduling_config();
	const auto normal = make_cat1_envelope(config);
	const auto overridden = make_cat1_envelope(config, true);
	TEST_EXPECT_NEAR(
		context,
		overridden.hard_protection.maximum_normal_acceleration_g -
			normal.hard_protection.maximum_normal_acceleration_g,
		2.0,
		kTolerance);
}

void test_stores_transition_is_continuous(Tests::Context& context)
{
	Systems::ModeAndGainScheduling scheduling(
		Systems::make_fck1c_mode_and_gain_scheduling_config());
	scheduling.set_stores_configuration(Systems::StoresConfiguration::Cat3);
	const auto first = scheduling.update({
		1.0 / 64.0, 5000.0, 0.7, true, false });
	TEST_EXPECT(context, first.stores_transition_0_1 > 0.0);
	TEST_EXPECT(context, first.stores_transition_0_1 < 1.0);
	TEST_EXPECT(
		context,
		first.stores_configuration == Systems::StoresConfiguration::Cat3);
}

void test_mode_scheduler_owns_f16xl_cruise_aoa_schedule(
	Tests::Context& context)
{
	Systems::ModeAndGainScheduling scheduling(
		Systems::make_fck1c_mode_and_gain_scheduling_config());
	const auto low_speed = scheduling.update({
		1.0 / 64.0, 5000.0, 0.0, true, false });
	TEST_EXPECT_NEAR(context,
		low_speed.envelope.hard_protection.angle_of_attack_blend_start_rad,
		Common::rad(19.0), kTolerance);
	TEST_EXPECT_NEAR(context,
		low_speed.envelope.hard_protection.angle_of_attack_limit_rad,
		Common::rad(29.0), kTolerance);
	const auto high_speed = scheduling.update({
		1.0 / 64.0, 5000.0, 1.0, true, false });
	TEST_EXPECT_NEAR(context,
		high_speed.envelope.hard_protection.angle_of_attack_limit_rad,
		Common::rad(26.0), kTolerance);
}

void test_mode_scheduler_owns_f16xl_gear_down_aoa_schedule(
	Tests::Context& context)
{
	Systems::ModeAndGainScheduling scheduling(
		Systems::make_fck1c_mode_and_gain_scheduling_config());
	const auto landing = scheduling.update({
		1.0 / 64.0, 5000.0, 0.0, true, true });
	TEST_EXPECT_NEAR(context,
		landing.envelope.hard_protection.angle_of_attack_blend_start_rad,
		Common::rad(10.0), kTolerance);
	TEST_EXPECT_NEAR(context,
		landing.envelope.hard_protection.angle_of_attack_limit_rad,
		Common::rad(16.0), kTolerance);
}

void test_cat_transition_does_not_change_controller_deadband(
	Tests::Context& context)
{
	const auto config =
		Systems::make_fck1c_mode_and_gain_scheduling_config();
	TEST_EXPECT_NEAR(context,
		config.cat1.pilot_input.deadband_normalized,
		config.cat3.pilot_input.deadband_normalized,
		kTolerance);
}
}

void run_configuration_and_mode_tests(Tests::Context& context)
{
	test_reference_guidance_envelope(context);
	test_guidance_must_fit_inside_hard_protection(context);
	test_developer_override_is_explicit(context);
	test_stores_transition_is_continuous(context);
	test_mode_scheduler_owns_f16xl_cruise_aoa_schedule(context);
	test_mode_scheduler_owns_f16xl_gear_down_aoa_schedule(context);
	test_cat_transition_does_not_change_controller_deadband(context);
}
