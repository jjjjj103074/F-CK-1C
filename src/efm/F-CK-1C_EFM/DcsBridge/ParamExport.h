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

inline double engine_display_related_rpm(double core_readout)
{
	const double idle_related_rpm = 0.675;
	const double clamped = Common::limit(core_readout, 0.0, 1.0);
	if (clamped <= 0.5)
	{
		return (clamped / 0.5) * idle_related_rpm;
	}
	return idle_related_rpm + ((clamped - 0.5) / 0.5) * (1.0 - idle_related_rpm);
}

inline double get_param(unsigned index, const ParamExportState& state)
{
	using namespace DcsIds::Params;
	const double nominal_core_rpm = 14710.0;
	const double nominal_fan_rpm = 8215.0;
	const double left_core_related_rpm = engine_display_related_rpm(state.left_engine_power_readout);
	const double right_core_related_rpm = engine_display_related_rpm(state.right_engine_power_readout);
	const double left_fan_related_rpm = state.left_engine_switch ? left_core_related_rpm : 0.0;
	const double right_fan_related_rpm = state.right_engine_switch ? right_core_related_rpm : 0.0;
	const double left_core_rpm = left_core_related_rpm * nominal_core_rpm;
	const double right_core_rpm = right_core_related_rpm * nominal_core_rpm;
	const double left_fan_rpm = left_fan_related_rpm * nominal_fan_rpm;
	const double right_fan_rpm = right_fan_related_rpm * nominal_fan_rpm;

	switch (index)
	{
		case NoseWheelYaw:
		{
			const bool wow = state.suspension_feedback_available && state.any_weight_on_wheels;
			const bool nws_valid = wow && (state.gear_pos > 0.5);
			return nws_valid ? state.nose_wheel_steering : 0.0;
		}

		case NoseWheelSpin:
			return state.wheel_spin[0];
		case LeftWheelSpin:
			return state.wheel_spin[1];
		case RightWheelSpin:
			return state.wheel_spin[2];

		case NoseBrakeMoment:
			return 0.0;
		case LeftBrakeMoment:
			return Common::limit(state.wheel_brake_left, 0.0, 1.0);
		case RightBrakeMoment:
			return Common::limit(state.wheel_brake_right, 0.0, 1.0);

		case AntiSkidEnable:
			return true;

		case StickPitch:
			return Common::limit(state.pitch_input, -1.0, 1.0);

		case StickRoll:
			return Common::limit(state.roll_input, -1.0, 1.0);

		case RudderPedals:
			return Common::limit(-state.yaw_input, -1.0, 1.0);

		case WheelBrakeLeft:
		case WheelBrakeCommandLeft:
			return Common::limit(state.wheel_brake_left, 0.0, 1.0);

		case WheelBrakeRight:
		case WheelBrakeCommandRight:
			return Common::limit(state.wheel_brake_right, 0.0, 1.0);

		case ThrottleLeft:
			if (state.left_engine_switch == false)
				return Common::limit(state.left_throttle_input, 0.0, 0.1);
			else
				return Common::limit(state.left_throttle_input, 0.1, 1.0);

		case ThrottleRight:
			if (state.right_engine_switch == false)
				return Common::limit(state.right_throttle_input, 0.0, 0.1);
			else
				return Common::limit(state.right_throttle_input, 0.1, 1.0);

		case InternalFuel:
			return state.internal_fuel;
		case TotalFuel:
			return state.total_fuel;

		case OxygenSupply:
			return 101000.0;

		case FlowVelocity:
			return 10.0;

		case NoseGearPostState:
		case LeftGearPostState:
		case RightGearPostState:
			return state.gear_pos;	// Landing gear states, combined

		// APU, doesn't make sounds.
		case ApuRpm:
		case ApuRelatedRpm:
			return 1;
		case ApuThrust:
		case ApuRelatedThrust:
			return 0;

		// Engine 1, left
		case LeftEngineCoreRpm:
			return left_core_rpm;
		case LeftEngineRpm:
			return left_fan_rpm;
		case LeftEngineCombustion:
			return state.left_engine_switch ? Common::limit(state.left_engine_power_readout * 2.0, 0.0, 1.0) : 0.0;

		case LeftEngineRelatedThrust:
			return state.left_throttle_output;
		case LeftEngineCoreRelatedThrust:
			return state.left_throttle_output;
		case LeftEngineRelatedRpm:
			return left_fan_related_rpm;
		case LeftEngineCoreRelatedRpm:
			return left_core_related_rpm;

		case LeftEngineCoreThrust:
		case LeftEngineThrust:
			return state.left_thrust_force;
		case LeftEngineTemperature:
			return (std::pow(state.left_engine_power_readout, 3) * 500) + state.atmosphere_temperature;

		// Engine 2, right
		case RightEngineCoreRpm:
			return right_core_rpm;
		case RightEngineRpm:
			return right_fan_rpm;
		case RightEngineCombustion:
			return state.right_engine_switch ? Common::limit(state.right_engine_power_readout * 2.0, 0.0, 1.0) : 0.0;

		case RightEngineRelatedThrust:
			return state.right_throttle_output;
		case RightEngineCoreRelatedThrust:
			return state.right_throttle_output;
		case RightEngineRelatedRpm:
			return right_fan_related_rpm;
		case RightEngineCoreRelatedRpm:
			return right_core_related_rpm;

		case RightEngineCoreThrust:
		case RightEngineThrust:
			return state.right_thrust_force;
		case RightEngineTemperature:
			return (std::pow(state.right_engine_power_readout, 3) * 500) + state.atmosphere_temperature;
	}
	return 0;
}
}
