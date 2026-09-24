#include "TestHarness.h"

#include "Common/Clamp.h"
#include "Common/Units.h"
#include "Core/Systems/FlightControlComputer/ControlLaws/ControlLaws.h"

#include <cmath>
#include <variant>

namespace
{
constexpr double kTolerance = 1e-9;
constexpr double kControlDtS = 1.0 / 64.0;

Systems::ActiveFlightControlConfiguration normal_configuration(
	bool landing_mode = false)
{
	Systems::ModeAndGainScheduling scheduling(
		Systems::make_fck1c_mode_and_gain_scheduling_config());
	return scheduling.update({
		kControlDtS, 5000.0, 0.0, true, landing_mode });
}

Systems::FlightControlLawsInput normal_input()
{
	Systems::FlightControlLawsInput input;
	input.flight.dt_s = kControlDtS;
	input.flight.indicated_airspeed_mps = 150.0;
	input.flight.dynamic_pressure_pa = 5000.0;
	input.flight.normal_acceleration_g = 1.0;
	input.maneuver.longitudinal.command =
		Core::Systems::NormalAccelerationCommand{ 1.0 };
	input.configuration = normal_configuration();
	return input;
}

Systems::FlightControlLawsResult prime_and_step(
	Systems::FlightControlLaws& laws,
	Systems::FlightControlLawsInput& input)
{
	(void)laws.update(input);
	return laws.update(input);
}

void set_normal_acceleration_command(
	Systems::FlightControlLawsInput& input,
	double target_g)
{
	input.maneuver.longitudinal.command =
		Core::Systems::NormalAccelerationCommand{ target_g };
}

void set_pitch_rate_command(
	Systems::FlightControlLawsInput& input,
	double target_rad_s)
{
	input.maneuver.longitudinal.command =
		Core::Systems::PitchRateCommand{ target_rad_s };
}

void test_direct_law_is_explicit_and_physical(Tests::Context& context)
{
	const Systems::FlightControlLawsConfig config;
	Systems::FlightControlLaws laws(config);
	auto input = normal_input();
	input.signals.pilot_pitch_raw_normalized = 0.5;
	input.signals.pilot_roll_raw_normalized = -0.25;
	input.signals.pilot_yaw_raw_normalized = 0.2;
	const auto result = laws.update(input);
	TEST_EXPECT_NEAR(context,
		result.developer_direct_surface_demand.symmetric_stabilator_demand_rad,
		0.5 * config.surface_mixer.symmetric_stabilator_limit_rad, kTolerance);
	TEST_EXPECT_NEAR(context,
		result.developer_direct_surface_demand.differential_flaperon_demand_rad,
		-0.25 * config.surface_mixer.differential_flaperon_limit_rad, kTolerance);
	TEST_EXPECT_NEAR(context,
		result.developer_direct_surface_demand.rudder_demand_rad,
		0.2 * config.surface_mixer.rudder_limit_rad, kTolerance);
}

void test_normal_law_maps_each_axis_to_named_surface(
	Tests::Context& context)
{
	Systems::FlightControlLaws laws({});
	auto input = normal_input();
	input.maneuver.lateral_directional.roll_rate_reference_rad_s = 0.2;
	input.maneuver.lateral_directional.yaw_rate_feedforward_rad_s = -0.1;
	set_normal_acceleration_command(input, 2.0);
	const auto result = prime_and_step(laws, input);
	TEST_EXPECT(context,
		result.normal_surface_demand.differential_flaperon_demand_rad > 0.0);
	TEST_EXPECT(context, result.normal_surface_demand.rudder_demand_rad < 0.0);
	TEST_EXPECT(context,
		result.normal_surface_demand.symmetric_stabilator_demand_rad > 0.0);
}

void test_normal_mode_uses_washed_out_q_as_feedback(
	Tests::Context& context)
{
	Systems::FlightControlLaws laws({});
	auto input = normal_input();
	(void)laws.update(input);
	input.flight.pitch_rate_rad_s = 0.4;
	const auto result = laws.update(input);
	TEST_EXPECT(context,
		result.diagnostics.longitudinal_command_mode ==
			Core::Systems::LongitudinalCommandMode::NormalAcceleration);
	TEST_EXPECT(context,
		result.diagnostics.pitch_rate_washout_feedback_effort < 0.0);
	TEST_EXPECT_NEAR(context,
		result.diagnostics.requested_pitch_rate_command_rad_s,
		0.0, kTolerance);
}

void test_alpha_command_limiter_reduces_positive_nz(
	Tests::Context& context)
{
	Systems::FlightControlLaws laws({});
	auto input = normal_input();
	input.flight.angle_of_attack_rad = Common::rad(24.0);
	set_normal_acceleration_command(input, 8.8);
	const auto result = prime_and_step(laws, input);
	TEST_EXPECT(context, result.status.angle_of_attack_limit_active);
	TEST_EXPECT(context, result.diagnostics.angle_of_attack_blend_0_1 > 0.0);
	TEST_EXPECT(context, result.diagnostics.angle_of_attack_blend_0_1 < 1.0);
	TEST_EXPECT(context,
		result.diagnostics.effective_normal_acceleration_reference_g < 8.8);
}

void test_alpha_limit_combines_nz_error_and_static_stability(
	Tests::Context& context)
{
	Systems::FlightControlLaws laws({});
	auto input = normal_input();
	input.flight.angle_of_attack_rad = Common::rad(29.0);
	input.flight.normal_acceleration_g = 2.4;
	set_normal_acceleration_command(input, 8.8);
	const auto result = prime_and_step(laws, input);
	TEST_EXPECT_NEAR(context,
		result.diagnostics.effective_normal_acceleration_reference_g,
		1.0, kTolerance);
	TEST_EXPECT(context,
		result.diagnostics.angle_of_attack_stability_feedback_effort < 0.0);
	TEST_EXPECT(context,
		result.normal_surface_demand.symmetric_stabilator_demand_rad < 0.0);
}

void test_alpha_limit_preserves_nose_down_command(Tests::Context& context)
{
	Systems::FlightControlLaws laws({});
	auto input = normal_input();
	input.flight.angle_of_attack_rad = Common::rad(29.0);
	set_normal_acceleration_command(input, -1.0);
	const auto result = prime_and_step(laws, input);
	TEST_EXPECT(context, !result.status.angle_of_attack_limit_active);
	TEST_EXPECT_NEAR(context,
		result.diagnostics.effective_normal_acceleration_reference_g,
		-1.0, kTolerance);
	TEST_EXPECT(context,
		result.normal_surface_demand.symmetric_stabilator_demand_rad < 0.0);
}

void test_alpha_schedule_is_smooth_and_monotonic(Tests::Context& context)
{
	auto evaluate = [](double alpha_deg)
	{
		Systems::FlightControlLaws laws({});
		auto input = normal_input();
		input.flight.angle_of_attack_rad = Common::rad(alpha_deg);
		set_normal_acceleration_command(input, 8.8);
		return prime_and_step(laws, input).diagnostics;
	};
	const auto start = evaluate(19.0);
	const auto middle = evaluate(24.0);
	const auto limit = evaluate(29.0);
	TEST_EXPECT_NEAR(
		context, start.angle_of_attack_blend_0_1, 0.0, kTolerance);
	TEST_EXPECT(context,
		middle.angle_of_attack_maximum_normal_acceleration_g <
			start.angle_of_attack_maximum_normal_acceleration_g);
	TEST_EXPECT(context,
		middle.angle_of_attack_maximum_normal_acceleration_g >
			limit.angle_of_attack_maximum_normal_acceleration_g);
	TEST_EXPECT_NEAR(context,
		limit.angle_of_attack_maximum_normal_acceleration_g, 1.0, kTolerance);
}

void test_pitch_rate_mode_limits_only_positive_command(
	Tests::Context& context)
{
	Systems::FlightControlLaws positive({});
	auto input = normal_input();
	input.configuration = normal_configuration(true);
	input.flight.angle_of_attack_rad = Common::rad(16.0);
	set_pitch_rate_command(input, 0.5);
	const auto limited = prime_and_step(positive, input);
	TEST_EXPECT_NEAR(context,
		limited.diagnostics.effective_pitch_rate_command_rad_s,
		0.0, kTolerance);
	Systems::FlightControlLaws negative({});
	set_pitch_rate_command(input, -0.5);
	const auto preserved = prime_and_step(negative, input);
	TEST_EXPECT_NEAR(context,
		preserved.diagnostics.effective_pitch_rate_command_rad_s,
		-0.5, kTolerance);
}

void test_longitudinal_mode_transfer_tracks_actual_surface(
	Tests::Context& context)
{
	const Systems::FlightControlLawsConfig config;
	Systems::FlightControlLaws laws(config);
	auto input = normal_input();
	set_normal_acceleration_command(input, 2.0);
	const auto normal = prime_and_step(laws, input);
	const double previous =
		normal.normal_surface_demand.symmetric_stabilator_demand_rad;
	input.signals.symmetric_stabilator_position_rad = previous;
	set_pitch_rate_command(input, 0.2);
	const auto transfer = laws.update(input);
	TEST_EXPECT(context, transfer.status.longitudinal_mode_transition_active);
	TEST_EXPECT_NEAR(context,
		transfer.normal_surface_demand.symmetric_stabilator_demand_rad,
		previous, kTolerance);
}

void test_protection_unwinds_sustained_pull_without_delayed_step(
	Tests::Context& context)
{
	Systems::FlightControlLaws laws({});
	auto input = normal_input();
	set_normal_acceleration_command(input, 8.8);
	for (int tick = 0; tick < 256; ++tick)
	{
		input.signals.symmetric_stabilator_saturated = true;
		input.signals.symmetric_stabilator_position_limit =
			Core::FlightControlPositionLimit::Positive;
		input.signals.symmetric_stabilator_at_position_limit = true;
		input.signals.symmetric_stabilator_position_rad = Common::rad(25.0);
		(void)laws.update(input);
	}
	double previous_effort = 1.0;
	for (int tick = 0; tick <= 128; ++tick)
	{
		input.flight.angle_of_attack_rad = Common::rad(
			10.0 + 19.0 * static_cast<double>(tick) / 128.0);
		input.flight.normal_acceleration_g = 1.0 +
			1.4 * static_cast<double>(tick) / 128.0;
		input.signals.symmetric_stabilator_saturated = false;
		input.signals.symmetric_stabilator_position_limit =
			Core::FlightControlPositionLimit::None;
		input.signals.symmetric_stabilator_at_position_limit = false;
		const auto result = laws.update(input);
		const double effort = result.diagnostics.limited_pitch_effort;
		TEST_EXPECT(context, effort <= previous_effort + 0.03);
		previous_effort = effort;
	}
	TEST_EXPECT(context, previous_effort < 0.0);
}

void test_alpha_protection_does_not_track_normal_servo_lag(
	Tests::Context& context)
{
	Systems::FlightControlLaws leading({});
	Systems::FlightControlLaws lagging({});
	auto leading_input = normal_input();
	leading_input.flight.angle_of_attack_rad = Common::rad(24.0);
	leading_input.flight.normal_acceleration_g = 2.4;
	set_normal_acceleration_command(leading_input, 8.8);
	auto lagging_input = leading_input;
	leading_input.signals.symmetric_stabilator_position_rad = Common::rad(8.0);
	lagging_input.signals.symmetric_stabilator_position_rad = Common::rad(-8.0);
	const auto first = leading.update(leading_input);
	const auto second = leading.update(leading_input);
	(void)lagging.update(lagging_input);
	const auto lagging_second = lagging.update(lagging_input);
	TEST_EXPECT(context, first.status.angle_of_attack_limit_active);
	TEST_EXPECT(context, second.status.anti_windup_active);
	TEST_EXPECT(context, !first.status.electronic_command_saturated);
	TEST_EXPECT_NEAR(context,
		second.diagnostics.longitudinal_integral_effort,
		lagging_second.diagnostics.longitudinal_integral_effort,
		kTolerance);
}

void test_alpha_protection_back_calculates_electronic_objective(
	Tests::Context& context)
{
	Systems::FlightControlLawsConfig tracking_config;
	Systems::FlightControlLawsConfig no_tracking_config = tracking_config;
	no_tracking_config.longitudinal.normal_acceleration_anti_windup_s_inv = 0.0;
	Systems::FlightControlLaws tracking(tracking_config);
	Systems::FlightControlLaws no_tracking(no_tracking_config);
	auto input = normal_input();
	input.flight.angle_of_attack_rad = Common::rad(24.0);
	input.flight.normal_acceleration_g = 2.4;
	set_normal_acceleration_command(input, 8.8);
	(void)tracking.update(input);
	const auto tracked = tracking.update(input);
	(void)no_tracking.update(input);
	const auto untracked = no_tracking.update(input);
	TEST_EXPECT_NEAR(context,
		tracked.diagnostics.longitudinal_integral_effort, 0.0,
		std::fabs(untracked.diagnostics.longitudinal_integral_effort));
}

void test_electronic_saturation_back_calculates_integral(
	Tests::Context& context)
{
	Systems::FlightControlLawsConfig tracking_config;
	Systems::FlightControlLawsConfig no_tracking_config = tracking_config;
	no_tracking_config.longitudinal.normal_acceleration_anti_windup_s_inv = 0.0;
	Systems::FlightControlLaws tracking(tracking_config);
	Systems::FlightControlLaws no_tracking(no_tracking_config);
	auto input = normal_input();
	input.flight.normal_acceleration_g = -2.0;
	set_normal_acceleration_command(input, 8.8);
	const auto first = tracking.update(input);
	const auto second = tracking.update(input);
	(void)no_tracking.update(input);
	const auto without_back_calculation = no_tracking.update(input);
	TEST_EXPECT(context, first.status.electronic_command_saturated);
	TEST_EXPECT(context, first.status.anti_windup_active);
	TEST_EXPECT(context,
		second.diagnostics.longitudinal_integral_effort <
			without_back_calculation.diagnostics.longitudinal_integral_effort);
}

void test_surface_demands_are_finite_and_bounded(Tests::Context& context)
{
	const Systems::FlightControlLawsConfig config;
	Systems::FlightControlLaws laws(config);
	auto input = normal_input();
	set_normal_acceleration_command(input, 100.0);
	input.maneuver.lateral_directional.roll_rate_reference_rad_s = 100.0;
	input.maneuver.lateral_directional.yaw_rate_feedforward_rad_s = -100.0;
	const auto result = prime_and_step(laws, input);
	TEST_EXPECT(context, std::isfinite(
		result.normal_surface_demand.symmetric_stabilator_demand_rad));
	TEST_EXPECT(context, std::fabs(
		result.normal_surface_demand.symmetric_stabilator_demand_rad) <=
		config.surface_mixer.symmetric_stabilator_limit_rad);
	TEST_EXPECT(context, std::fabs(
		result.normal_surface_demand.differential_flaperon_demand_rad) <=
		config.surface_mixer.differential_flaperon_limit_rad);
	TEST_EXPECT(context, std::fabs(
		result.normal_surface_demand.rudder_demand_rad) <=
		config.surface_mixer.rudder_limit_rad);
}
}

void run_fbw_controller_tests(Tests::Context& context)
{
	test_direct_law_is_explicit_and_physical(context);
	test_normal_law_maps_each_axis_to_named_surface(context);
	test_normal_mode_uses_washed_out_q_as_feedback(context);
	test_alpha_command_limiter_reduces_positive_nz(context);
	test_alpha_limit_combines_nz_error_and_static_stability(context);
	test_alpha_limit_preserves_nose_down_command(context);
	test_alpha_schedule_is_smooth_and_monotonic(context);
	test_pitch_rate_mode_limits_only_positive_command(context);
	test_longitudinal_mode_transfer_tracks_actual_surface(context);
	test_protection_unwinds_sustained_pull_without_delayed_step(context);
	test_alpha_protection_does_not_track_normal_servo_lag(context);
	test_alpha_protection_back_calculates_electronic_objective(context);
	test_electronic_saturation_back_calculates_integral(context);
	test_surface_demands_are_finite_and_bounded(context);
}
