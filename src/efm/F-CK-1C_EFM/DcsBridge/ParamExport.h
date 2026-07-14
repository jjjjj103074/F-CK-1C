#pragma once

#include "../Common/Clamp.h"
#include "../DcsIds/ParamIds.h"
#include <cmath>

namespace DcsBridge
{
struct ParamExportState
{
	bool suspension_feedback_available;
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

struct ParamLookup
{
	bool found = false;
	double value = 0.0;
};

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

inline ParamLookup lookup_wheel_motion(unsigned index, const ParamExportState& state)
{
	using namespace DcsIds::Params;
	switch (index)
	{
	case NoseWheelYaw:
	{
		const bool wow = state.suspension_feedback_available && state.any_weight_on_wheels;
		return { true, wow && state.gear_pos > 0.5 ? state.nose_wheel_steering : 0.0 };
	}
	case NoseWheelSpin: return { true, state.wheel_spin[0] };
	case LeftWheelSpin: return { true, state.wheel_spin[1] };
	case RightWheelSpin: return { true, state.wheel_spin[2] };
	case NoseGearPostState:
	case LeftGearPostState:
	case RightGearPostState: return { true, state.gear_pos };
	default: return {};
	}
}

inline ParamLookup lookup_wheel_brakes(unsigned index, const ParamExportState& state)
{
	using namespace DcsIds::Params;
	switch (index)
	{
	case NoseBrakeMoment: return { true, 0.0 };
	case LeftBrakeMoment:
	case WheelBrakeLeft:
	case WheelBrakeCommandLeft:
		return { true, Common::limit(state.wheel_brake_left, 0.0, 1.0) };
	case RightBrakeMoment:
	case WheelBrakeRight:
	case WheelBrakeCommandRight:
		return { true, Common::limit(state.wheel_brake_right, 0.0, 1.0) };
	case AntiSkidEnable: return { true, 1.0 };
	default: return {};
	}
}

inline ParamLookup lookup_flight_controls(unsigned index, const ParamExportState& state)
{
	using namespace DcsIds::Params;
	switch (index)
	{
	case StickPitch: return { true, Common::limit(state.pitch_input, -1.0, 1.0) };
	case StickRoll: return { true, Common::limit(state.roll_input, -1.0, 1.0) };
	case RudderPedals: return { true, Common::limit(-state.yaw_input, -1.0, 1.0) };
	case ThrottleLeft:
		return { true, state.left_engine_switch
			? Common::limit(state.left_throttle_input, 0.1, 1.0)
			: Common::limit(state.left_throttle_input, 0.0, 0.1) };
	case ThrottleRight:
		return { true, state.right_engine_switch
			? Common::limit(state.right_throttle_input, 0.1, 1.0)
			: Common::limit(state.right_throttle_input, 0.0, 0.1) };
	default: return {};
	}
}

inline ParamLookup lookup_aircraft_services(unsigned index, const ParamExportState& state)
{
	using namespace DcsIds::Params;
	switch (index)
	{
	case InternalFuel: return { true, state.internal_fuel };
	case TotalFuel: return { true, state.total_fuel };
	case OxygenSupply: return { true, 101000.0 };
	case FlowVelocity: return { true, 10.0 };
	case ApuRpm:
	case ApuRelatedRpm: return { true, 1.0 };
	case ApuThrust:
	case ApuRelatedThrust: return { true, 0.0 };
	default: return {};
	}
}

inline double engine_temperature(double power_readout, double atmosphere_temperature)
{
	return std::pow(power_readout, kEngineTemperatureExponent) *
		kEngineTemperatureScale + atmosphere_temperature;
}

inline ParamLookup lookup_left_engine_speed(
	unsigned index,
	const EngineDisplayState& display)
{
	using namespace DcsIds::Params;
	switch (index)
	{
	case LeftEngineCoreRpm: return { true, display.left_core_rpm };
	case LeftEngineRpm: return { true, display.left_fan_rpm };
	case LeftEngineRelatedRpm: return { true, display.left_fan_related_rpm };
	case LeftEngineCoreRelatedRpm: return { true, display.left_core_related_rpm };
	default: return {};
	}
}

inline ParamLookup lookup_left_engine_output(
	unsigned index,
	const ParamExportState& state)
{
	using namespace DcsIds::Params;
	switch (index)
	{
	case LeftEngineCombustion:
		return { true, state.left_engine_switch
			? Common::limit(state.left_engine_power_readout * kEngineCombustionScale, 0.0, 1.0)
			: 0.0 };
	case LeftEngineRelatedThrust:
	case LeftEngineCoreRelatedThrust: return { true, state.left_throttle_output };
	case LeftEngineCoreThrust:
	case LeftEngineThrust: return { true, state.left_thrust_force };
	case LeftEngineTemperature:
		return { true, engine_temperature(
			state.left_engine_power_readout, state.atmosphere_temperature) };
	default: return {};
	}
}

inline ParamLookup lookup_right_engine_speed(
	unsigned index,
	const EngineDisplayState& display)
{
	using namespace DcsIds::Params;
	switch (index)
	{
	case RightEngineCoreRpm: return { true, display.right_core_rpm };
	case RightEngineRpm: return { true, display.right_fan_rpm };
	case RightEngineRelatedRpm: return { true, display.right_fan_related_rpm };
	case RightEngineCoreRelatedRpm: return { true, display.right_core_related_rpm };
	default: return {};
	}
}

inline ParamLookup lookup_right_engine_output(
	unsigned index,
	const ParamExportState& state)
{
	using namespace DcsIds::Params;
	switch (index)
	{
	case RightEngineCombustion:
		return { true, state.right_engine_switch
			? Common::limit(state.right_engine_power_readout * kEngineCombustionScale, 0.0, 1.0)
			: 0.0 };
	case RightEngineRelatedThrust:
	case RightEngineCoreRelatedThrust: return { true, state.right_throttle_output };
	case RightEngineCoreThrust:
	case RightEngineThrust: return { true, state.right_thrust_force };
	case RightEngineTemperature:
		return { true, engine_temperature(
			state.right_engine_power_readout, state.atmosphere_temperature) };
	default: return {};
	}
}

inline double get_param(unsigned index, const ParamExportState& state)
{
	ParamLookup result = lookup_wheel_motion(index, state);
	if (result.found) return result.value;
	result = lookup_wheel_brakes(index, state);
	if (result.found) return result.value;
	result = lookup_flight_controls(index, state);
	if (result.found) return result.value;
	result = lookup_aircraft_services(index, state);
	if (result.found) return result.value;
	const EngineDisplayState display = make_engine_display_state(state);
	result = lookup_left_engine_speed(index, display);
	if (result.found) return result.value;
	result = lookup_left_engine_output(index, state);
	if (result.found) return result.value;
	result = lookup_right_engine_speed(index, display);
	if (result.found) return result.value;
	result = lookup_right_engine_output(index, state);
	return result.found ? result.value : 0.0;
}
}
