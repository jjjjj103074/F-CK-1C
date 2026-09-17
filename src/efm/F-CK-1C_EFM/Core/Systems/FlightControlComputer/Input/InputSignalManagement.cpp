#include "InputSignalManagement.h"

#include "Common/Angles.h"
#include "Common/Clamp.h"

#include <cmath>
#include <stdexcept>

namespace
{
constexpr double kMinimumTimeConstantS = 1.0e-6;

void require_finite(double value, const char* field)
{
	if (!std::isfinite(value)) throw std::invalid_argument(field);
}

void require_normalized(double value, const char* field)
{
	require_finite(value, field);
	if (value < -1.0 || value > 1.0) throw std::out_of_range(field);
}
}

namespace Core
{
namespace Systems
{
InputSignalManagement::InputSignalManagement(
	const ::Systems::InputSignalManagementConfig& config)
	: config_(config)
{
}

const ::Systems::ManagedFlightControlSignals& InputSignalManagement::update(
	const InputSignalManagementStepInput& input)
{
	validate(input.raw);
	state_.dt_s = input.raw.dt_s;
	update_observation(input.raw);
	update_pilot_raw(input.raw.pilot);
	update_pitch_shaping(input.pilot_shaping, input.raw.dt_s);
	if (input.update_lateral_directional_shaping)
	{
		update_lateral_directional_shaping(
			input.pilot_shaping,
			input.lateral_directional_shaping_dt_s);
	}
	const double deadband = input.pilot_shaping.deadband_normalized;
	state_.roll_pitch_in_deadband =
		std::fabs(state_.pilot_roll_raw_normalized) <= deadband &&
		std::fabs(state_.pilot_pitch_raw_normalized) <= deadband;
	state_.pitch_in_deadband =
		std::fabs(state_.pilot_pitch_raw_normalized) <= deadband;
	update_actuator_feedback(input.raw);
	return state_;
}

double InputSignalManagement::filter(const FilterInput& input) const
{
	if (input.time_constant_s <= kMinimumTimeConstantS) return input.target;
	const double gain = Common::limit(
		input.dt_s / (input.time_constant_s + input.dt_s), 0.0, 1.0);
	return input.current + (input.target - input.current) * gain;
}

double InputSignalManagement::filter_signed_angle(
	const FilterInput& input) const
{
	const double delta_rad = Common::shortest_angle_difference_rad(
		input.target, input.current);
	return Common::wrap_signed_angle_rad(filter({
		input.current, input.current + delta_rad,
		input.time_constant_s, input.dt_s }));
}

double InputSignalManagement::filter_heading_deg(
	const FilterInput& input) const
{
	const double delta_deg = Common::shortest_heading_difference_deg(
		input.target, input.current);
	return Common::wrap_heading_deg(filter({
		input.current, input.current + delta_deg,
		input.time_constant_s, input.dt_s }));
}

double InputSignalManagement::shape_stick(
	double value,
	double cubic_weight) const
{
	return (1.0 - cubic_weight) * value +
		cubic_weight * value * value * value;
}

void InputSignalManagement::validate(const RawFlightControlInput& raw) const
{
	require_finite(raw.dt_s, "flight-control dt");
	if (raw.dt_s <= 0.0) throw std::out_of_range("flight-control dt");
	const FlightControlObservation& value = raw.observation;
	const double observations[] = {
		value.indicated_airspeed_mps,
		value.vertical_speed_ft_s, value.mach, value.dynamic_pressure_pa,
		value.normal_acceleration_g, value.angle_of_attack_rad,
		value.sideslip_rad, value.roll_rad, value.pitch_rad,
		value.roll_rate_rad_s, value.pitch_rate_rad_s, value.yaw_rate_rad_s
	};
	for (double observation : observations)
		require_finite(observation, "flight-control observation");
	if (value.magnetic_heading_available)
		require_finite(value.magnetic_heading_deg, "magnetic heading");
	if (value.pressure_altitude_available)
		require_finite(value.pressure_altitude_ft, "pressure altitude");
	require_normalized(raw.pilot.pitch_axis_normalized, "pitch axis");
	require_normalized(raw.pilot.roll_axis_normalized, "roll axis");
	require_normalized(raw.pilot.yaw_axis_normalized, "yaw axis");
}

void InputSignalManagement::update_observation(
	const RawFlightControlInput& raw)
{
	const FlightControlObservation& source = raw.observation;
	if (!observation_initialized_)
	{
		initialize_observation(source);
		return;
	}
	update_navigation_observation(source, raw.dt_s);
	update_motion_observation(source, raw.dt_s);
	update_air_data_observation(source, raw.dt_s);
}

void InputSignalManagement::initialize_observation(
	const FlightControlObservation& source)
{
	state_.observation = source;
	if (source.magnetic_heading_available)
	{
		state_.observation.magnetic_heading_deg =
			Common::wrap_heading_deg(source.magnetic_heading_deg);
	}
	observation_initialized_ = true;
}

void InputSignalManagement::update_navigation_observation(
	const FlightControlObservation& source,
	double dt_s)
{
	FlightControlObservation& output = state_.observation;
	const double tau_s = config_.signal_filter_time_constant_s;
	if (source.pressure_altitude_available)
	{
		output.pressure_altitude_ft = output.pressure_altitude_available
			? filter({ output.pressure_altitude_ft,
				source.pressure_altitude_ft, tau_s, dt_s })
			: source.pressure_altitude_ft;
	}
	output.pressure_altitude_available =
		source.pressure_altitude_available;
	output.vertical_speed_ft_s = filter(
		{ output.vertical_speed_ft_s, source.vertical_speed_ft_s,
			tau_s, dt_s });
	if (source.magnetic_heading_available)
	{
		output.magnetic_heading_deg = output.magnetic_heading_available
			? filter_heading_deg({ output.magnetic_heading_deg,
				source.magnetic_heading_deg, tau_s, dt_s })
			: Common::wrap_heading_deg(source.magnetic_heading_deg);
	}
	output.magnetic_heading_available = source.magnetic_heading_available;
	output.roll_rad = filter_signed_angle(
		{ output.roll_rad, source.roll_rad, tau_s, dt_s });
	output.pitch_rad = filter(
		{ output.pitch_rad, source.pitch_rad, tau_s, dt_s });
}

void InputSignalManagement::update_motion_observation(
	const FlightControlObservation& source,
	double dt_s)
{
	FlightControlObservation& output = state_.observation;
	const double tau_s = config_.signal_filter_time_constant_s;
	output.roll_rate_rad_s = filter(
		{ output.roll_rate_rad_s, source.roll_rate_rad_s, tau_s, dt_s });
	output.pitch_rate_rad_s = filter(
		{ output.pitch_rate_rad_s, source.pitch_rate_rad_s, tau_s, dt_s });
	output.yaw_rate_rad_s = filter(
		{ output.yaw_rate_rad_s, source.yaw_rate_rad_s, tau_s, dt_s });
	output.angle_of_attack_rad = filter(
		{ output.angle_of_attack_rad, source.angle_of_attack_rad,
			tau_s, dt_s });
	output.sideslip_rad = filter(
		{ output.sideslip_rad, source.sideslip_rad, tau_s, dt_s });
}

void InputSignalManagement::update_air_data_observation(
	const FlightControlObservation& source,
	double dt_s)
{
	FlightControlObservation& output = state_.observation;
	const double tau_s = config_.signal_filter_time_constant_s;
	output.dynamic_pressure_pa = filter({ output.dynamic_pressure_pa,
		source.dynamic_pressure_pa,
		config_.dynamic_pressure_filter_time_constant_s, dt_s });
	output.indicated_airspeed_mps = filter({ output.indicated_airspeed_mps,
		source.indicated_airspeed_mps, tau_s, dt_s });
	output.mach = filter({ output.mach, source.mach, tau_s, dt_s });
	output.normal_acceleration_g = filter({ output.normal_acceleration_g,
		source.normal_acceleration_g,
		config_.normal_acceleration_filter_time_constant_s, dt_s });
}

void InputSignalManagement::update_pilot_raw(
	const PilotControlSignal& pilot)
{
	state_.pilot_roll_raw_normalized = Common::limit(
		pilot.roll_axis_normalized + pilot.roll_trim_normalized, -1.0, 1.0);
	state_.pilot_pitch_raw_normalized = Common::limit(
		pilot.pitch_axis_normalized + pilot.pitch_trim_normalized, -1.0, 1.0);
	state_.pilot_yaw_raw_normalized = Common::limit(
		pilot.yaw_axis_normalized + pilot.yaw_trim_normalized, -1.0, 1.0);
}

double InputSignalManagement::shape_axis(const ShapeAxisInput& input) const
{
	const double maximum_step =
		input.config.command_rate_normalized_s * input.dt_s;
	return Common::limit(
		filter({ input.current, input.target,
			input.config.command_time_constant_s, input.dt_s }),
		input.current - maximum_step, input.current + maximum_step);
}

void InputSignalManagement::update_pitch_shaping(
	const ::Systems::PilotInputShapingConfig& config,
	double dt_s)
{
	const double pitch = shape_stick(
		state_.pilot_pitch_raw_normalized, config.cubic_weight);
	state_.pilot_pitch_normalized = shape_axis({
		state_.pilot_pitch_normalized, pitch, config, dt_s });
}

void InputSignalManagement::update_lateral_directional_shaping(
	const ::Systems::PilotInputShapingConfig& config,
	double dt_s)
{
	const double roll = shape_stick(
		state_.pilot_roll_raw_normalized, config.cubic_weight);
	const double yaw = shape_stick(
		state_.pilot_yaw_raw_normalized, config.cubic_weight);
	state_.pilot_roll_normalized = shape_axis({
		state_.pilot_roll_normalized, roll, config, dt_s });
	state_.pilot_yaw_normalized = shape_axis({
		state_.pilot_yaw_normalized, yaw, config, dt_s });
}

void InputSignalManagement::update_actuator_feedback(
	const RawFlightControlInput& raw)
{
	state_.gear_position_normalized = raw.landing_gear.position_normalized;
	state_.landing_gear_handle_down = raw.landing_gear.handle_down;
	state_.weight_on_wheels = raw.landing_gear.any_weight_on_wheels;
	state_.symmetric_stabilator_position_rad =
		raw.actuator.symmetric_stabilator.position_rad;
	state_.symmetric_stabilator_position_limit =
		raw.actuator.symmetric_stabilator.position_limit;
	state_.symmetric_stabilator_at_position_limit =
		raw.actuator.symmetric_stabilator.at_position_limit;
	state_.symmetric_stabilator_saturated =
		raw.actuator.symmetric_stabilator.saturated;
	state_.differential_flaperon_position_rad =
		raw.actuator.differential_flaperon.position_rad;
	state_.rudder_position_rad = raw.actuator.rudder.position_rad;
	state_.actuator_saturated = raw.actuator.any_saturated;
}
}
}
