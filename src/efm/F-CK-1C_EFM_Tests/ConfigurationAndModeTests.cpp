#include "TestHarness.h"

#include "Common/Units.h"
#include "Core/Systems/FlightControlComputer/ControlLaws/ConfigurationAndMode.h"

#include <stdexcept>

namespace
{
constexpr double kTolerance = 1e-9;

void test_reference_guidance_envelope(Tests::Context& context)
{
	Systems::FBWControllerConfig config;
	const Systems::ManeuverEnvelope envelope =
		Systems::make_maneuver_envelope(
			config, { config.cat1, 20.0, false });
	TEST_EXPECT_NEAR(
		context, envelope.guidance.bank_limit_rad, Common::rad(30.0), kTolerance);
	TEST_EXPECT_NEAR(
		context,
		envelope.guidance.roll_rate_limit_rad_s,
		Common::rad(20.0),
		kTolerance);
	TEST_EXPECT_NEAR(
		context, envelope.guidance.minimum_normal_acceleration_g, 0.5, kTolerance);
	TEST_EXPECT_NEAR(
		context, envelope.guidance.maximum_normal_acceleration_g, 2.0, kTolerance);
}

void test_guidance_must_fit_inside_hard_protection(Tests::Context& context)
{
	Systems::FBWControllerConfig config;
	config.guidance_roll_rate_limit_rad_s = config.cat3.p_rate_limit + 0.1;
	bool rejected = false;
	try
	{
		(void)Systems::make_maneuver_envelope(
			config, { config.cat3, 20.0, false });
	}
	catch (const std::invalid_argument&)
	{
		rejected = true;
	}
	TEST_EXPECT(context, rejected);
}

void test_developer_override_is_explicit(Tests::Context& context)
{
	Systems::FBWControllerConfig config;
	const auto normal = Systems::make_maneuver_envelope(
		config, { config.cat1, 20.0, false });
	const auto overridden = Systems::make_maneuver_envelope(
		config, { config.cat1, 20.0, true });
	TEST_EXPECT_NEAR(
		context,
		overridden.hard_protection.maximum_normal_acceleration_g -
			normal.hard_protection.maximum_normal_acceleration_g,
		2.0,
		kTolerance);
}
}

void run_configuration_and_mode_tests(Tests::Context& context)
{
	test_reference_guidance_envelope(context);
	test_guidance_must_fit_inside_hard_protection(context);
	test_developer_override_is_explicit(context);
}
