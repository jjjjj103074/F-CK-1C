#include "TestHarness.h"

#include "Common/Clamp.h"
#include "Common/Units.h"
#include "Core/Systems/FlightControlActuationSystem/FlightControlActuationSystem.h"
#include "Core/Systems/FlightControlComputer/FlightControlComputer.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
constexpr double kFccDtS = 1.0 / 64.0;
constexpr double kActuatorDtS = 1.0 / 256.0;
constexpr int kActuatorStepsPerFccTick = 4;
constexpr double kGravityMps2 = 9.80665;
constexpr double kTwo = 2.0;
constexpr double kTestAirspeedMps = 150.0;
constexpr double kLowTestAirspeedMps = 130.0;
constexpr double kHighTestAirspeedMps = 220.0;
constexpr double kPitchAccelerationGain = 3.0;
constexpr double kPitchRateDamping = 2.2;
constexpr double kNormalAccelerationGain = 2.2;
constexpr double kRollAccelerationGain = 5.0;
constexpr double kRollRateDamping = 2.8;
constexpr double kVerticalSpeedDamping = 0.04;
constexpr double kInitialAltitudeM = 2000.0;
constexpr double kAltitudeStepM = 30.48;
constexpr double kHeadingStepRad = Common::kPi / 180.0;
constexpr int kHeadingStepCount = 10;
constexpr int kTrimTicks = 128;
constexpr int kResponseTicks = 64 * 30;
constexpr int kTailWindowTicks = 64 * 5;
constexpr int kTailHalfWindowTicks = kTailWindowTicks / 2;
constexpr double kAltitudeSettlingBandM = 2.0;
constexpr double kHeadingSettlingBandRad = Common::kPi / 180.0;
constexpr double kMaximumTailOscillationM = 20.0;
constexpr double kTailDecayRatio = 0.95;
constexpr double kComparisonTolerance = 1e-9;
constexpr double kRiseErrorFraction = 0.1;
constexpr double kControlDeltaNoiseFloor = 1e-5;
constexpr double kNeutralControlNormalized = 0.5;
constexpr double kPressedCommandValue = 1.0;
constexpr double kNormalizedCommandLimit = 1.0;
constexpr double kMaximumAuthorityTransitionCommandStep = 0.25;
constexpr double kLevelNormalAccelerationG = 1.0;
constexpr double kTestMach = 0.5;
constexpr double kTestDynamicPressurePa = 5000.0;
constexpr double kTestAlphaDeg = 3.0;
constexpr double kTestAlphaLimitDeg = 20.0;

double wrap_pi(double angle_rad)
{
	while (angle_rad > Common::kPi) angle_rad -= kTwo * Common::kPi;
	while (angle_rad < -Common::kPi) angle_rad += kTwo * Common::kPi;
	return angle_rad;
}

struct SmallSignalAircraft
{
	double altitude_m = kInitialAltitudeM;
	double vertical_speed_mps = 0.0;
	double heading_rad = 0.0;
	double roll_rad = 0.0;
	double pitch_rad = 0.0;
	double roll_rate_rad_s = 0.0;
	double pitch_rate_rad_s = 0.0;
	double yaw_rate_rad_s = 0.0;
	double normal_acceleration_g = kLevelNormalAccelerationG;
};

class ClosedLoopRig final
{
public:
	explicit ClosedLoopRig(double airspeed_mps = kTestAirspeedMps)
		: computer_(
			Core::Systems::fck1c_flight_control_computer_config(),
			Core::StartMode::HotAir,
			{ kNeutralControlNormalized, kNeutralControlNormalized }),
		  actuation_(
			Core::Systems::fck1c_flight_control_actuation_system_config()),
		  airspeed_mps_(airspeed_mps)
	{
	}

	void command(Core::CommandId id, double value = kPressedCommandValue)
	{
		computer_.handle_command({ id, value });
	}

	void tick()
	{
		const auto& demand = computer_.step(
			{ make_input(),
				{ kNeutralControlNormalized, kNeutralControlNormalized } });
		for (int index = 0; index < kActuatorStepsPerFccTick; ++index)
		{
			(void)actuation_.update(demand, kActuatorDtS);
		}
		update_aircraft();
	}

	const SmallSignalAircraft& aircraft() const { return aircraft_; }
	const Core::AutomaticFlightControlSnapshot& autopilot() const
	{
		return computer_.automatic_flight_control_snapshot();
	}
	const Core::FlightControlComputerSnapshot& diagnostics() const
	{
		return computer_.diagnostics();
	}
	bool actuator_saturated() const
	{
		return actuation_.state().any_saturated;
	}

private:
	Core::Systems::RawFlightControlInput make_input() const
	{
		Core::Systems::RawFlightControlInput input;
		input.dt_s = kFccDtS;
		input.alpha_limit_deg = kTestAlphaLimitDeg;
		input.observation = {
			aircraft_.altitude_m, airspeed_mps_,
			aircraft_.vertical_speed_mps, kTestMach, kTestDynamicPressurePa,
			aircraft_.normal_acceleration_g, kTestAlphaDeg, 0.0,
			aircraft_.heading_rad, aircraft_.roll_rad, aircraft_.pitch_rad,
			aircraft_.roll_rate_rad_s, aircraft_.pitch_rate_rad_s,
			aircraft_.yaw_rate_rad_s
		};
		input.actuator = actuation_.state();
		return input;
	}

	void update_aircraft()
	{
		const auto& surface = actuation_.state();
		const double elevator = surface.elevator.normalized_position;
		const double aileron = surface.aileron.normalized_position;
		aircraft_.pitch_rate_rad_s +=
			(kPitchAccelerationGain * elevator -
				kPitchRateDamping * aircraft_.pitch_rate_rad_s) * kFccDtS;
		aircraft_.roll_rate_rad_s +=
			(kRollAccelerationGain * aileron -
				kRollRateDamping * aircraft_.roll_rate_rad_s) * kFccDtS;
		aircraft_.pitch_rad += aircraft_.pitch_rate_rad_s * kFccDtS;
		aircraft_.roll_rad += aircraft_.roll_rate_rad_s * kFccDtS;
		aircraft_.normal_acceleration_g =
			kLevelNormalAccelerationG + kNormalAccelerationGain * elevator;
		const double vertical_acceleration_mps2 = kGravityMps2 *
			(aircraft_.normal_acceleration_g * std::cos(aircraft_.roll_rad) -
				kLevelNormalAccelerationG) -
			kVerticalSpeedDamping * aircraft_.vertical_speed_mps;
		aircraft_.vertical_speed_mps +=
			vertical_acceleration_mps2 * kFccDtS;
		aircraft_.altitude_m += aircraft_.vertical_speed_mps * kFccDtS;
		aircraft_.yaw_rate_rad_s =
			kGravityMps2 * std::tan(aircraft_.roll_rad) / airspeed_mps_;
		aircraft_.heading_rad = wrap_pi(
			aircraft_.heading_rad + aircraft_.yaw_rate_rad_s * kFccDtS);
	}

	Core::Systems::FlightControlComputer computer_;
	Core::Systems::FlightControlActuationSystem actuation_;
	SmallSignalAircraft aircraft_;
	const double airspeed_mps_;
};

struct AxisActivityState
{
	double previous_command = 0.0;
	double previous_delta = 0.0;
};

struct SampleRange
{
	double minimum = std::numeric_limits<double>::max();
	double maximum = std::numeric_limits<double>::lowest();

	void record(double value)
	{
		minimum = std::min(minimum, value);
		maximum = std::max(maximum, value);
	}

	double amplitude() const { return maximum - minimum; }
};

struct ResponseMetrics
{
	double initial_altitude_error_m = kAltitudeStepM;
	double final_altitude_error_m = 0.0;
	double initial_heading_error_rad = kHeadingStepRad * kHeadingStepCount;
	double final_heading_error_rad = 0.0;
	SampleRange early_tail_altitude;
	SampleRange late_tail_altitude;
	SampleRange early_tail_heading;
	SampleRange late_tail_heading;
	double maximum_surface_command = 0.0;
	double maximum_altitude_overshoot_m = 0.0;
	double settling_time_s = kResponseTicks * kFccDtS;
	double rise_time_s = kResponseTicks * kFccDtS;
	double control_activity = 0.0;
	double maximum_surface_rate_per_s = 0.0;
	AxisActivityState elevator_activity;
	AxisActivityState aileron_activity;
	int saturation_ticks = 0;
	int surface_reversals = 0;
	int last_unsettled_tick = kResponseTicks - 1;
	bool finite = true;
	bool rise_reached = false;
	bool vertical_degraded = false;
	bool lateral_degraded = false;
};

void record_axis_activity(double command, AxisActivityState& axis,
	ResponseMetrics& metrics)
{
	const double delta = command - axis.previous_command;
	metrics.control_activity += std::abs(delta);
	metrics.maximum_surface_rate_per_s = std::max(
		metrics.maximum_surface_rate_per_s, std::abs(delta) / kFccDtS);
	if (std::abs(delta) > kControlDeltaNoiseFloor)
	{
		if (delta * axis.previous_delta < 0.0) ++metrics.surface_reversals;
		axis.previous_delta = delta;
	}
	axis.previous_command = command;
}

void issue_combined_capture_commands(ClosedLoopRig& rig, bool vertical_first)
{
	rig.command(Core::CommandId::EngageAutopilot);
	const auto vertical = [&rig]() {
		rig.command(Core::CommandId::SelectAutopilotAltitudeHold);
		rig.command(Core::CommandId::IncreaseAutopilotVerticalReference);
	};
	const auto lateral = [&rig]() {
		rig.command(Core::CommandId::SelectAutopilotHeading);
		for (int index = 0; index < kHeadingStepCount; ++index)
			rig.command(Core::CommandId::IncreaseAutopilotLateralReference);
	};
	if (vertical_first) vertical();
	lateral();
	if (!vertical_first) vertical();
}

void record_response(ResponseMetrics& metrics, const ClosedLoopRig& rig, int tick)
{
	const auto& aircraft = rig.aircraft();
	const auto& diagnostics = rig.diagnostics();
	const double elevator = diagnostics.elevator_command_normalized;
	const double aileron = diagnostics.aileron_command_normalized;
	const double altitude_error =
		kInitialAltitudeM + kAltitudeStepM - aircraft.altitude_m;
	const double heading_error = wrap_pi(
		kHeadingStepRad * kHeadingStepCount - aircraft.heading_rad);
	const double surface_command =
		std::max(std::abs(elevator), std::abs(aileron));
	metrics.finite = metrics.finite && std::isfinite(aircraft.altitude_m) &&
		std::isfinite(aircraft.heading_rad) && std::isfinite(elevator) &&
		std::isfinite(aileron);
	metrics.maximum_surface_command = std::max(
		metrics.maximum_surface_command, surface_command);
	metrics.maximum_altitude_overshoot_m = std::max(
		metrics.maximum_altitude_overshoot_m, -altitude_error);
	record_axis_activity(elevator, metrics.elevator_activity, metrics);
	record_axis_activity(aileron, metrics.aileron_activity, metrics);
	if (rig.actuator_saturated())
		++metrics.saturation_ticks;
	if (!metrics.rise_reached &&
		std::abs(altitude_error) <= kAltitudeStepM * kRiseErrorFraction &&
		std::abs(heading_error) <=
			kHeadingStepRad * kHeadingStepCount * kRiseErrorFraction)
	{
		metrics.rise_time_s = (tick + 1) * kFccDtS;
		metrics.rise_reached = true;
	}
	if (std::abs(altitude_error) > kAltitudeSettlingBandM ||
		std::abs(heading_error) > kHeadingSettlingBandRad)
		metrics.last_unsettled_tick = tick;
	const int tail_start_tick = kResponseTicks - kTailWindowTicks;
	if (tick < tail_start_tick) return;
	const bool early_half = tick < tail_start_tick + kTailHalfWindowTicks;
	SampleRange& altitude_range = early_half
		? metrics.early_tail_altitude : metrics.late_tail_altitude;
	SampleRange& heading_range = early_half
		? metrics.early_tail_heading : metrics.late_tail_heading;
	altitude_range.record(aircraft.altitude_m);
	heading_range.record(aircraft.heading_rad);
}

ResponseMetrics run_combined_capture(ClosedLoopRig& rig, bool vertical_first)
{
	for (int tick = 0; tick < kTrimTicks; ++tick) rig.tick();
	issue_combined_capture_commands(rig, vertical_first);
	ResponseMetrics metrics;
	for (int tick = 0; tick < kResponseTicks; ++tick)
	{
		rig.tick();
		record_response(metrics, rig, tick);
	}
	metrics.final_altitude_error_m =
		kInitialAltitudeM + kAltitudeStepM - rig.aircraft().altitude_m;
	metrics.final_heading_error_rad = wrap_pi(
		kHeadingStepRad * kHeadingStepCount - rig.aircraft().heading_rad);
	metrics.settling_time_s =
		(metrics.last_unsettled_tick + 1) * kFccDtS;
	metrics.vertical_degraded = rig.autopilot().vertical_degraded;
	metrics.lateral_degraded = rig.autopilot().lateral_degraded;
	return metrics;
}

void expect_stable_response(
	Tests::Context& context,
	const ResponseMetrics& metrics)
{
	TEST_EXPECT(context, metrics.finite);
	TEST_EXPECT(context, metrics.rise_reached);
	TEST_EXPECT(
		context,
		std::abs(metrics.final_altitude_error_m) <
			metrics.initial_altitude_error_m);
	TEST_EXPECT(
		context,
		std::abs(metrics.final_heading_error_rad) <
			metrics.initial_heading_error_rad);
	TEST_EXPECT(
		context,
		metrics.maximum_surface_command <= kNormalizedCommandLimit);
	TEST_EXPECT(context, metrics.saturation_ticks == 0);
	TEST_EXPECT(context, std::isfinite(metrics.control_activity));
	TEST_EXPECT(context, std::isfinite(metrics.maximum_surface_rate_per_s));
	TEST_EXPECT(
		context,
		metrics.late_tail_altitude.amplitude() <
			kMaximumTailOscillationM);
	const bool altitude_tail_decays =
		metrics.late_tail_altitude.amplitude() <= kAltitudeSettlingBandM ||
		metrics.late_tail_altitude.amplitude() <
			metrics.early_tail_altitude.amplitude() * kTailDecayRatio;
	const bool heading_tail_decays =
		metrics.late_tail_heading.amplitude() <= kHeadingSettlingBandRad ||
		metrics.late_tail_heading.amplitude() <
			metrics.early_tail_heading.amplitude() * kTailDecayRatio;
	TEST_EXPECT(context, altitude_tail_decays);
	TEST_EXPECT(context, heading_tail_decays);
	TEST_EXPECT(context, !metrics.vertical_degraded);
	TEST_EXPECT(context, !metrics.lateral_degraded);
}

void expect_bounded_surface_step(
	Tests::Context& context,
	const Core::FlightControlComputerSnapshot& before,
	const Core::FlightControlComputerSnapshot& after)
{
	TEST_EXPECT(
		context,
		std::abs(after.elevator_command_normalized -
			before.elevator_command_normalized) <=
			kMaximumAuthorityTransitionCommandStep);
	TEST_EXPECT(
		context,
		std::abs(after.aileron_command_normalized -
			before.aileron_command_normalized) <=
			kMaximumAuthorityTransitionCommandStep);
}

void test_combined_capture_is_finite_and_convergent(Tests::Context& context)
{
	ClosedLoopRig rig;
	const auto metrics = run_combined_capture(rig, true);
	expect_stable_response(context, metrics);
	TEST_EXPECT(context, metrics.settling_time_s < kResponseTicks * kFccDtS);
}

void test_capture_is_stable_across_airspeeds(Tests::Context& context)
{
	ClosedLoopRig low_speed(kLowTestAirspeedMps);
	ClosedLoopRig high_speed(kHighTestAirspeedMps);
	const auto low = run_combined_capture(low_speed, true);
	const auto high = run_combined_capture(high_speed, true);
	expect_stable_response(context, low);
	expect_stable_response(context, high);
}

void test_axis_command_order_does_not_change_result(Tests::Context& context)
{
	ClosedLoopRig vertical_first;
	ClosedLoopRig lateral_first;
	const auto first = run_combined_capture(vertical_first, true);
	const auto second = run_combined_capture(lateral_first, false);
	TEST_EXPECT_NEAR(
		context, first.final_altitude_error_m, second.final_altitude_error_m,
		kComparisonTolerance);
	TEST_EXPECT_NEAR(
		context, first.final_heading_error_rad, second.final_heading_error_rad,
		kComparisonTolerance);
}

void test_cat3_capture_remains_stable(Tests::Context& context)
{
	ClosedLoopRig rig;
	rig.command(Core::CommandId::SetFbwCat3);
	expect_stable_response(context, run_combined_capture(rig, true));
}

void test_selected_source_diagnostics_follow_authority(
	Tests::Context& context)
{
	ClosedLoopRig rig;
	rig.tick();
	TEST_EXPECT(
		context,
		rig.diagnostics().selected_longitudinal_source ==
			Core::FlightControlReferenceSource::Manual);
	rig.command(Core::CommandId::EngageAutopilot);
	rig.tick();
	TEST_EXPECT(
		context,
		rig.diagnostics().selected_longitudinal_source ==
			Core::FlightControlReferenceSource::Automatic);
	TEST_EXPECT(
		context,
		rig.diagnostics().selected_lateral_source ==
			Core::FlightControlReferenceSource::Automatic);
	rig.command(Core::CommandId::SetAutopilotBypass);
	rig.tick();
	TEST_EXPECT(
		context,
		rig.diagnostics().selected_longitudinal_source ==
			Core::FlightControlReferenceSource::Manual);
	rig.command(Core::CommandId::SetAutopilotBypass, 0.0);
	rig.tick();
	TEST_EXPECT(
		context,
		rig.diagnostics().selected_longitudinal_source ==
			Core::FlightControlReferenceSource::Automatic);
}

void test_authority_transitions_do_not_create_command_steps(
	Tests::Context& context)
{
	ClosedLoopRig rig;
	for (int tick = 0; tick < kTrimTicks; ++tick) rig.tick();
	auto before = rig.diagnostics();
	rig.command(Core::CommandId::EngageAutopilot);
	rig.tick();
	auto after = rig.diagnostics();
	expect_bounded_surface_step(context, before, after);
	before = after;
	rig.command(Core::CommandId::SetAutopilotBypass);
	rig.tick();
	after = rig.diagnostics();
	expect_bounded_surface_step(context, before, after);
	before = after;
	rig.command(Core::CommandId::SetAutopilotBypass, 0.0);
	rig.tick();
	after = rig.diagnostics();
	expect_bounded_surface_step(context, before, after);
	before = after;
	rig.command(Core::CommandId::DisengageAutopilot);
	rig.tick();
	expect_bounded_surface_step(context, before, rig.diagnostics());
}
}

void run_autopilot_closed_loop_tests(Tests::Context& context)
{
	test_combined_capture_is_finite_and_convergent(context);
	test_capture_is_stable_across_airspeeds(context);
	test_axis_command_order_does_not_change_result(context);
	test_cat3_capture_remains_stable(context);
	test_selected_source_diagnostics_follow_authority(context);
	test_authority_transitions_do_not_create_command_steps(context);
}
