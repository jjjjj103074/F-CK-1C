#include "TestHarness.h"

#include "Common/Units.h"
#include "Core/Systems/FlightControlComputer/ControlLaws/ConfigurationAndMode.h"
#include "Core/Systems/FlightControlComputer/ControlLaws/ControlLawMath.h"
#include "Core/Systems/FlightControlComputer/ControlLaws/ControlReferenceSelection.h"
#include "Core/Systems/FlightControlComputer/ControlLaws/GuidanceCoordination.h"
#include "Core/Systems/FlightControlComputer/ControlLaws/PilotCommandLaw.h"

#include <cmath>

namespace
{
constexpr double kTolerance = 1e-9;
constexpr double kTestAirspeedMps = 150.0;

Systems::ConditionedFlightControlInput nominal_flight()
{
	Systems::ConditionedFlightControlInput flight;
	flight.dt_s = 1.0 / 64.0;
	flight.dynamic_pressure_pa = 5000.0;
	flight.alpha_limit_deg = 20.0;
	flight.indicated_airspeed_mps = kTestAirspeedMps;
	flight.normal_acceleration_g = 1.0;
	return flight;
}

Systems::ManeuverEnvelope envelope(
	const Systems::FBWControllerConfig& config)
{
	return Systems::make_maneuver_envelope(
		config, { config.cat1, 20.0, false });
}

Core::Systems::GuidanceCoordinationInput automatic_input()
{
	Systems::FBWControllerConfig config;
	Core::Systems::AutomaticFlightGuidanceReference automatic;
	automatic.longitudinal_authority =
		Core::Systems::AuthorityState::Automatic;
	automatic.lateral_authority =
		Core::Systems::AuthorityState::Automatic;
	automatic.vertical_type =
		Core::Systems::VerticalGuidanceReferenceType::VerticalSpeed;
	automatic.bank_angle_reference_rad = Common::rad(30.0);
	const Core::Systems::CoordinatedManeuverReference manual;
	return {
		nominal_flight(),
		Core::Systems::select_flight_reference(manual, automatic),
		envelope(config),
		config
	};
}

void test_pilot_mapping_uses_physical_references(Tests::Context& context)
{
	Systems::FBWControllerConfig config;
	Systems::ConditionedFlightControlInput flight = nominal_flight();
	flight.pilot_pitch_normalized = 0.5;
	flight.pilot_roll_normalized = -0.4;
	flight.pilot_yaw_normalized = 0.3;
	const Systems::FBWGainScheduleValues gains =
		Systems::fbw_eval_gain_schedule(config, flight.dynamic_pressure_pa);
	const auto result = Systems::make_pilot_maneuver_reference(
		{ flight, config, config.cat1, gains, envelope(config) });
	TEST_EXPECT(context, result.longitudinal.normal_acceleration_reference_g > 1.0);
	TEST_EXPECT(context, result.longitudinal.pitch_rate_feedforward_rad_s > 0.0);
	TEST_EXPECT(context, result.lateral_directional.roll_rate_reference_rad_s < 0.0);
	TEST_EXPECT(context, result.lateral_directional.yaw_rate_feedforward_rad_s > 0.0);
}

void test_reference_selection_preserves_axis_authority(
	Tests::Context& context)
{
	Core::Systems::CoordinatedManeuverReference manual;
	manual.longitudinal.normal_acceleration_reference_g = 1.5;
	Core::Systems::AutomaticFlightGuidanceReference automatic;
	automatic.longitudinal_authority =
		Core::Systems::AuthorityState::Automatic;
	automatic.lateral_authority = Core::Systems::AuthorityState::Bypassed;
	const auto selected =
		Core::Systems::select_flight_reference(manual, automatic);
	TEST_EXPECT(
		context,
		selected.longitudinal_authority ==
			Core::Systems::AuthorityState::Automatic);
	TEST_EXPECT(
		context,
		selected.lateral_authority == Core::Systems::AuthorityState::Bypassed);
	TEST_EXPECT_NEAR(
		context,
		selected.manual.longitudinal.normal_acceleration_reference_g,
		1.5,
		kTolerance);
}

void test_level_turn_compensation_is_applied_once(Tests::Context& context)
{
	const auto result = Core::Systems::coordinate_guidance(automatic_input());
	const double expected_nz_g = 1.0 / std::cos(Common::rad(30.0));
	TEST_EXPECT_NEAR(
		context,
		result.reference.longitudinal.normal_acceleration_reference_g,
		expected_nz_g,
		kTolerance);
	TEST_EXPECT(
		context,
		result.constraint.reason == Core::Systems::ConstraintReason::None);
}

void test_joint_feasibility_reports_limiting_axis(Tests::Context& context)
{
	auto input = automatic_input();
	input.selected.automatic.vertical_speed_reference_mps = 22.0;
	const auto result = Core::Systems::coordinate_guidance(input);
	TEST_EXPECT(
		context,
		result.constraint.reason ==
			Core::Systems::ConstraintReason::LateralConstrainedByVerticalAuthority);
	TEST_EXPECT(context, result.constraint.lateral_constrained);
	TEST_EXPECT(
		context,
		result.reference.longitudinal.normal_acceleration_reference_g <=
			input.envelope.guidance.maximum_normal_acceleration_g);
}

void test_vertical_overload_keeps_vertical_reason(Tests::Context& context)
{
	auto input = automatic_input();
	input.selected.automatic.vertical_speed_reference_mps = 30.0;
	const auto result = Core::Systems::coordinate_guidance(input);
	TEST_EXPECT(
		context,
		result.constraint.reason ==
			Core::Systems::ConstraintReason::VerticalReferenceUnmaintainable);
	TEST_EXPECT(context, result.constraint.vertical_constrained);
}

void test_coordinated_turn_preserves_manual_yaw(Tests::Context& context)
{
	auto input = automatic_input();
	const double manual_yaw_rate_rad_s = 0.2;
	input.selected.manual.lateral_directional.yaw_rate_feedforward_rad_s =
		manual_yaw_rate_rad_s;
	const auto result = Core::Systems::coordinate_guidance(input);
	TEST_EXPECT(
		context,
		result.reference.lateral_directional.yaw_rate_feedforward_rad_s >
			manual_yaw_rate_rad_s);
	TEST_EXPECT(
		context,
		result.reference.directional_authority ==
			Core::Systems::AuthorityState::Manual);
}
}

void run_flight_control_reference_tests(Tests::Context& context)
{
	test_pilot_mapping_uses_physical_references(context);
	test_reference_selection_preserves_axis_authority(context);
	test_level_turn_compensation_is_applied_once(context);
	test_joint_feasibility_reports_limiting_axis(context);
	test_vertical_overload_keeps_vertical_reason(context);
	test_coordinated_turn_preserves_manual_yaw(context);
}
