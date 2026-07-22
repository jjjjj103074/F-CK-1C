#pragma once

#include "../Common/Clamp.h"
#include "../DcsIds/ParamIds.h"
#include <cmath>
#include <optional>

namespace DcsBridge
{
struct ParamExportState
{
	bool suspension_feedback_available;
	bool atmosphere_available;
	bool any_weight_on_wheels;
	double gear_pos;
	double nose_wheel_steering;
	double wheel_spin[3];
	double wheel_brake_left;
	double wheel_brake_right;
	double pitch_input;
	double roll_input;
	double yaw_input;
	bool left_engine_switch;
	bool right_engine_switch;
	double left_throttle_input;
	double right_throttle_input;
	double left_throttle_output;
	double right_throttle_output;
	double left_engine_power_readout;
	double right_engine_power_readout;
	double left_thrust_force;
	double right_thrust_force;
	double atmosphere_temperature;
	double internal_fuel;
	double total_fuel;
};

enum class ParamDataCategory
{
	Atmosphere,
	Suspension
};

inline const char* param_data_category_name(ParamDataCategory category)
{
	switch (category)
	{
	case ParamDataCategory::Atmosphere: return "atmosphere";
	case ParamDataCategory::Suspension: return "suspension";
	}
	return "unknown";
}

inline std::optional<ParamDataCategory> missing_param_data(
	unsigned index,
	const ParamExportState& state)
{
	using namespace DcsIds::Params;
	switch (index)
	{
	case NoseWheelYaw:
		if (!state.suspension_feedback_available)
		{
			return ParamDataCategory::Suspension;
		}
		return std::nullopt;
	case LeftEngineTemperature:
	case RightEngineTemperature:
		if (!state.atmosphere_available)
		{
			return ParamDataCategory::Atmosphere;
		}
		return std::nullopt;
	default:
		return std::nullopt;
	}
}

struct EngineDisplayState
{
	double left_core_related_rpm = 0.0;
	double right_core_related_rpm = 0.0;
	double left_fan_related_rpm = 0.0;
	double right_fan_related_rpm = 0.0;
	double left_core_rpm = 0.0;
	double right_core_rpm = 0.0;
	double left_fan_rpm = 0.0;
	double right_fan_rpm = 0.0;
};

constexpr double kIdleRelatedRpm = 0.675;
constexpr double kNominalCoreRpm = 14710.0;
constexpr double kNominalFanRpm = 8215.0;
constexpr double kEngineCombustionScale = 2.0;
constexpr double kEngineTemperatureScale = 500.0;
constexpr double kEngineTemperatureExponent = 3.0;

inline double engine_display_related_rpm(double core_readout)
{
	const double clamped = Common::limit(core_readout, 0.0, 1.0);
	if (clamped <= 0.5)
	{
		return (clamped / 0.5) * kIdleRelatedRpm;
	}
	return kIdleRelatedRpm + ((clamped - 0.5) / 0.5) * (1.0 - kIdleRelatedRpm);
}

inline EngineDisplayState make_engine_display_state(const ParamExportState& state)
{
	EngineDisplayState display;
	display.left_core_related_rpm = engine_display_related_rpm(
		state.left_engine_power_readout);
	display.right_core_related_rpm = engine_display_related_rpm(
		state.right_engine_power_readout);
	display.left_fan_related_rpm = state.left_engine_switch
		? display.left_core_related_rpm : 0.0;
	display.right_fan_related_rpm = state.right_engine_switch
		? display.right_core_related_rpm : 0.0;
	display.left_core_rpm = display.left_core_related_rpm * kNominalCoreRpm;
	display.right_core_rpm = display.right_core_related_rpm * kNominalCoreRpm;
	display.left_fan_rpm = display.left_fan_related_rpm * kNominalFanRpm;
	display.right_fan_rpm = display.right_fan_related_rpm * kNominalFanRpm;
	return display;
}

inline std::optional<double> lookup_wheel_motion(
	unsigned index,
	const ParamExportState& state)
{
	using namespace DcsIds::Params;
	switch (index)
	{
	case NoseWheelYaw:
	{
		const bool wow = state.suspension_feedback_available && state.any_weight_on_wheels;
		return wow && state.gear_pos > 0.5 ? state.nose_wheel_steering : 0.0;
	}
	case NoseWheelSpin: return state.wheel_spin[0];
	case LeftWheelSpin: return state.wheel_spin[1];
	case RightWheelSpin: return state.wheel_spin[2];
	case NoseGearPostState:
	case LeftGearPostState:
	case RightGearPostState: return state.gear_pos;
	default: return std::nullopt;
	}
}

inline std::optional<double> lookup_wheel_brakes(
	unsigned index,
	const ParamExportState& state)
{
	using namespace DcsIds::Params;
	switch (index)
	{
	case NoseBrakeMoment: return 0.0;
	case LeftBrakeMoment:
	case WheelBrakeLeft:
	case WheelBrakeCommandLeft:
		return Common::limit(state.wheel_brake_left, 0.0, 1.0);
	case RightBrakeMoment:
	case WheelBrakeRight:
	case WheelBrakeCommandRight:
		return Common::limit(state.wheel_brake_right, 0.0, 1.0);
	case AntiSkidEnable: return 1.0;
	default: return std::nullopt;
	}
}

inline std::optional<double> lookup_flight_controls(
	unsigned index,
	const ParamExportState& state)
{
	using namespace DcsIds::Params;
	switch (index)
	{
	case StickPitch: return Common::limit(state.pitch_input, -1.0, 1.0);
	case StickRoll: return Common::limit(state.roll_input, -1.0, 1.0);
	case RudderPedals: return Common::limit(-state.yaw_input, -1.0, 1.0);
	case ThrottleLeft:
		return state.left_engine_switch
			? Common::limit(state.left_throttle_input, 0.1, 1.0)
			: Common::limit(state.left_throttle_input, 0.0, 0.1);
	case ThrottleRight:
		return state.right_engine_switch
			? Common::limit(state.right_throttle_input, 0.1, 1.0)
			: Common::limit(state.right_throttle_input, 0.0, 0.1);
	default: return std::nullopt;
	}
}

inline std::optional<double> lookup_aircraft_services(
	unsigned index,
	const ParamExportState& state)
{
	using namespace DcsIds::Params;
	switch (index)
	{
	case InternalFuel: return state.internal_fuel;
	case TotalFuel: return state.total_fuel;
	case OxygenSupply: return 101000.0;
	case FlowVelocity: return 10.0;
	case ApuRpm:
	case ApuRelatedRpm: return 1.0;
	case ApuThrust:
	case ApuRelatedThrust: return 0.0;
	default: return std::nullopt;
	}
}

inline double engine_temperature(double power_readout, double atmosphere_temperature)
{
	return std::pow(power_readout, kEngineTemperatureExponent) *
		kEngineTemperatureScale + atmosphere_temperature;
}

inline std::optional<double> lookup_left_engine_speed(
	unsigned index,
	const EngineDisplayState& display)
{
	using namespace DcsIds::Params;
	switch (index)
	{
	case LeftEngineCoreRpm: return display.left_core_rpm;
	case LeftEngineRpm: return display.left_fan_rpm;
	case LeftEngineRelatedRpm: return display.left_fan_related_rpm;
	case LeftEngineCoreRelatedRpm: return display.left_core_related_rpm;
	default: return std::nullopt;
	}
}

inline std::optional<double> lookup_left_engine_output(
	unsigned index,
	const ParamExportState& state)
{
	using namespace DcsIds::Params;
	switch (index)
	{
	case LeftEngineCombustion:
		return state.left_engine_switch
			? Common::limit(state.left_engine_power_readout * kEngineCombustionScale, 0.0, 1.0)
			: 0.0;
	case LeftEngineRelatedThrust:
	case LeftEngineCoreRelatedThrust: return state.left_throttle_output;
	case LeftEngineCoreThrust:
	case LeftEngineThrust: return state.left_thrust_force;
	case LeftEngineTemperature:
		return engine_temperature(
			state.left_engine_power_readout, state.atmosphere_temperature);
	default: return std::nullopt;
	}
}

inline std::optional<double> lookup_right_engine_speed(
	unsigned index,
	const EngineDisplayState& display)
{
	using namespace DcsIds::Params;
	switch (index)
	{
	case RightEngineCoreRpm: return display.right_core_rpm;
	case RightEngineRpm: return display.right_fan_rpm;
	case RightEngineRelatedRpm: return display.right_fan_related_rpm;
	case RightEngineCoreRelatedRpm: return display.right_core_related_rpm;
	default: return std::nullopt;
	}
}

inline std::optional<double> lookup_right_engine_output(
	unsigned index,
	const ParamExportState& state)
{
	using namespace DcsIds::Params;
	switch (index)
	{
	case RightEngineCombustion:
		return state.right_engine_switch
			? Common::limit(state.right_engine_power_readout * kEngineCombustionScale, 0.0, 1.0)
			: 0.0;
	case RightEngineRelatedThrust:
	case RightEngineCoreRelatedThrust: return state.right_throttle_output;
	case RightEngineCoreThrust:
	case RightEngineThrust: return state.right_thrust_force;
	case RightEngineTemperature:
		return engine_temperature(
			state.right_engine_power_readout, state.atmosphere_temperature);
	default: return std::nullopt;
	}
}

inline std::optional<double> get_param(
	unsigned index,
	const ParamExportState& state)
{
	std::optional<double> result = lookup_wheel_motion(index, state);
	if (result) return result;
	result = lookup_wheel_brakes(index, state);
	if (result) return result;
	result = lookup_flight_controls(index, state);
	if (result) return result;
	result = lookup_aircraft_services(index, state);
	if (result) return result;
	const EngineDisplayState display = make_engine_display_state(state);
	result = lookup_left_engine_speed(index, display);
	if (result) return result;
	result = lookup_left_engine_output(index, state);
	if (result) return result;
	result = lookup_right_engine_speed(index, display);
	if (result) return result;
	return lookup_right_engine_output(index, state);
}
}
