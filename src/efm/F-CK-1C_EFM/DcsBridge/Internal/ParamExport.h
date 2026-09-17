#pragma once

#include "../../Common/Clamp.h"
#include "../../Core/Contracts/FrameContracts.h"
#include "../../DcsIds/ParamIds.h"
#include <cmath>
#include <optional>

namespace DcsBridge
{
struct ParamExportState
{
	bool suspension_feedback_available;
	bool atmosphere_available;
	bool any_weight_on_wheels;
	double gear_position_normalized;
	double nose_wheel_steering_normalized;
	double wheel_spin_phase_0_1[3];
	double wheel_brake_left_normalized;
	double wheel_brake_right_normalized;
	double pitch_input_normalized;
	double roll_input_normalized;
	double yaw_input_normalized;
	bool left_engine_switch;
	bool right_engine_switch;
	double left_throttle_input_normalized;
	double right_throttle_input_normalized;
	double left_throttle_output_normalized;
	double right_throttle_output_normalized;
	double left_engine_power_readout_normalized;
	double right_engine_power_readout_normalized;
	double left_thrust_force_n;
	double right_thrust_force_n;
	double atmosphere_temperature_k;
	double internal_fuel_kg;
	double total_fuel_kg;
	double total_fuel_flow_kg_s;
};

inline ParamExportState make_param_export_state(const Core::FrameOutput& output)
{
	const bool suspension_available =
		output.availability.suspension[0] ||
		output.availability.suspension[1] ||
		output.availability.suspension[2];
	return {
		suspension_available,
		output.availability.atmosphere,
		output.suspension.any_weight_on_wheels,
		output.landing_gear.gear_position_normalized,
		output.landing_gear.nose_wheel_steering_normalized,
		{
			output.landing_gear.wheel_spin_phase_0_1[0],
			output.landing_gear.wheel_spin_phase_0_1[1],
			output.landing_gear.wheel_spin_phase_0_1[2]
		},
		output.landing_gear.brake_left_normalized,
		output.landing_gear.brake_right_normalized,
		output.controls.pitch_input_normalized,
		output.controls.roll_input_normalized,
		output.controls.yaw_input_normalized,
		output.engines[0].switch_on,
		output.engines[1].switch_on,
		output.engines[0].throttle_input_normalized,
		output.engines[1].throttle_input_normalized,
		output.engines[0].throttle_output_normalized,
		output.engines[1].throttle_output_normalized,
		output.engines[0].power_readout_normalized,
		output.engines[1].power_readout_normalized,
		output.engines[0].thrust_force_n,
		output.engines[1].thrust_force_n,
		output.flight.atmosphere_temperature_k,
		output.fuel.internal_fuel_kg,
		output.fuel.total_fuel_kg,
		output.fuel.total_fuel_flow_kg_s
	};
}

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
	double left_core_related_rpm_0_1 = 0.0;
	double right_core_related_rpm_0_1 = 0.0;
	double left_fan_related_rpm_0_1 = 0.0;
	double right_fan_related_rpm_0_1 = 0.0;
	double left_core_rpm = 0.0;
	double right_core_rpm = 0.0;
	double left_fan_rpm = 0.0;
	double right_fan_rpm = 0.0;
};

constexpr double kIdleRelatedRpm0To1 = 0.675;
constexpr double kNominalCoreRpm = 14710.0;
constexpr double kNominalFanRpm = 8215.0;
constexpr double kEngineCombustionScale = 2.0;
constexpr double kEngineTemperatureRiseC = 500.0;
constexpr double kEngineTemperatureExponent = 3.0;
constexpr double kKelvinToCelsiusOffset = 273.15;
constexpr double kPerEngineFuelFlowShare = 0.5;
constexpr double kUnsupportedAltimeterPressureCompatibilityMmHg = 760.0;

inline double engine_display_related_rpm(double core_readout)
{
	const double clamped = Common::limit(core_readout, 0.0, 1.0);
	if (clamped <= 0.5)
	{
		return (clamped / 0.5) * kIdleRelatedRpm0To1;
	}
	return kIdleRelatedRpm0To1 +
		((clamped - 0.5) / 0.5) * (1.0 - kIdleRelatedRpm0To1);
}

inline EngineDisplayState make_engine_display_state(const ParamExportState& state)
{
	EngineDisplayState display;
	display.left_core_related_rpm_0_1 = engine_display_related_rpm(
		state.left_engine_power_readout_normalized);
	display.right_core_related_rpm_0_1 = engine_display_related_rpm(
		state.right_engine_power_readout_normalized);
	display.left_fan_related_rpm_0_1 = state.left_engine_switch
		? display.left_core_related_rpm_0_1 : 0.0;
	display.right_fan_related_rpm_0_1 = state.right_engine_switch
		? display.right_core_related_rpm_0_1 : 0.0;
	display.left_core_rpm =
		display.left_core_related_rpm_0_1 * kNominalCoreRpm;
	display.right_core_rpm =
		display.right_core_related_rpm_0_1 * kNominalCoreRpm;
	display.left_fan_rpm =
		display.left_fan_related_rpm_0_1 * kNominalFanRpm;
	display.right_fan_rpm =
		display.right_fan_related_rpm_0_1 * kNominalFanRpm;
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
		return wow && state.gear_position_normalized > 0.5
			? state.nose_wheel_steering_normalized : 0.0;
	}
	case NoseWheelSpin: return state.wheel_spin_phase_0_1[0];
	case LeftWheelSpin: return state.wheel_spin_phase_0_1[1];
	case RightWheelSpin: return state.wheel_spin_phase_0_1[2];
	case NoseGearPostState:
	case LeftGearPostState:
	case RightGearPostState: return state.gear_position_normalized;
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
		return Common::limit(state.wheel_brake_left_normalized, 0.0, 1.0);
	case RightBrakeMoment:
	case WheelBrakeRight:
	case WheelBrakeCommandRight:
		return Common::limit(state.wheel_brake_right_normalized, 0.0, 1.0);
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
	case StickPitch:
		return Common::limit(state.pitch_input_normalized, -1.0, 1.0);
	case StickRoll:
		return Common::limit(state.roll_input_normalized, -1.0, 1.0);
	case RudderPedals:
		return Common::limit(-state.yaw_input_normalized, -1.0, 1.0);
	case ThrottleLeft:
		return state.left_engine_switch
			? Common::limit(state.left_throttle_input_normalized, 0.1, 1.0)
			: Common::limit(state.left_throttle_input_normalized, 0.0, 0.1);
	case ThrottleRight:
		return state.right_engine_switch
			? Common::limit(state.right_throttle_input_normalized, 0.1, 1.0)
			: Common::limit(state.right_throttle_input_normalized, 0.0, 0.1);
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
	case InternalFuel: return state.internal_fuel_kg;
	case TotalFuel: return state.total_fuel_kg;
	case OxygenSupply: return 101000.0;
	case FlowVelocity: return 10.0;
	case ApuRpm:
	case ApuRelatedRpm: return 1.0;
	case ApuThrust:
	case ApuRelatedThrust: return 0.0;
	default: return std::nullopt;
	}
}

inline std::optional<double> lookup_engine_compatibility(
	unsigned index,
	const ParamExportState& state)
{
	using namespace DcsIds::Params;
	switch (index)
	{
	case ApuCoreRelatedRpm:
		// Compatibility only: F-CK-1C does not model an APU.
		return 1.0;
	case LeftEngineFuelFlow:
	case RightEngineFuelFlow:
		// Temporary even split until per-engine fuel flow is implemented.
		return state.total_fuel_flow_kg_s * kPerEngineFuelFlowShare;
	case LeftPropellerPitch:
	case RightPropellerPitch:
		// The F-CK-1C uses turbofans, not propellers.
		return 0.0;
	case LeftEngineFanPhase:
	case RightEngineFanPhase:
		// Fan rotational phase is not implemented.
		return 0.0;
	case LeftEngineFlowSpeedCompatibility:
	case RightEngineFlowSpeedCompatibility:
		// The bundled SDK leaves these engine flow-speed indices unnamed.
		return 0.0;
	default: return std::nullopt;
	}
}

inline std::optional<double> lookup_wheel_compatibility(unsigned index)
{
	using namespace DcsIds::Params;
	switch (index)
	{
	case LeftMainWheelYawCompatibility:
	case RightMainWheelYawCompatibility:
		// Compatibility only: independent main-wheel yaw is not implemented.
		return 0.0;
	case LeftWheelSpin:
	case RightWheelSpin:
		// Main-wheel self-attitude is not implemented as a Param output.
		return 0.0;
	default: return std::nullopt;
	}
}

inline std::optional<double> lookup_force_feedback_compatibility(unsigned index)
{
	using namespace DcsIds::Params;
	switch (index)
	{
	case PitchForceCenter:
	case PitchForceFactor:
	case PitchForceShakeAmplitude:
	case PitchForceShakeFrequency:
	case RollForceCenter:
	case RollForceFactor:
	case RollForceShakeAmplitude:
	case RollForceShakeFrequency:
		// Force-feedback Param outputs are not implemented.
		return 0.0;
	default: return std::nullopt;
	}
}

inline std::optional<double> lookup_misc_system_compatibility(unsigned index)
{
	using namespace DcsIds::Params;
	switch (index)
	{
	case CockpitPressurization:
		// The aircraft currently has no cockpit pressurization model.
		return 0.0;
	case CockpitAltimeterPressureSetting:
		// Unsupported compatibility output: the cockpit has no dynamic
		// altimeter-pressure setting. Standard atmosphere is not live state.
		return kUnsupportedAltimeterPressureCompatibilityMmHg;
	case InterruptRefuel:
		// The EFM does not request interruption of refueling.
		return 0.0;
	case UnknownCompatibility2134:
	case UnknownCompatibility2135:
	case UnknownCompatibility2136:
	case UnknownCompatibility2137:
		// DCS queries these indices, but the bundled SDK does not define them.
		return 0.0;
	default: return std::nullopt;
	}
}

inline std::optional<double> lookup_system_compatibility(unsigned index)
{
	std::optional<double> result = lookup_wheel_compatibility(index);
	if (result) return result;
	result = lookup_force_feedback_compatibility(index);
	if (result) return result;
	return lookup_misc_system_compatibility(index);
}

inline double engine_temperature_c(
	double power_readout_normalized,
	double atmosphere_temperature_k)
{
	const double atmosphere_temperature_c =
		atmosphere_temperature_k - kKelvinToCelsiusOffset;
	return std::pow(
		power_readout_normalized,
		kEngineTemperatureExponent) * kEngineTemperatureRiseC +
		atmosphere_temperature_c;
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
	case LeftEngineRelatedRpm: return display.left_fan_related_rpm_0_1;
	case LeftEngineCoreRelatedRpm: return display.left_core_related_rpm_0_1;
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
			? Common::limit(
				state.left_engine_power_readout_normalized *
					kEngineCombustionScale,
				0.0,
				1.0)
			: 0.0;
	case LeftEngineRelatedThrust:
	case LeftEngineCoreRelatedThrust:
		return state.left_throttle_output_normalized;
	case LeftEngineCoreThrust:
	case LeftEngineThrust: return state.left_thrust_force_n;
	case LeftEngineTemperature:
		return engine_temperature_c(
			state.left_engine_power_readout_normalized,
			state.atmosphere_temperature_k);
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
	case RightEngineRelatedRpm: return display.right_fan_related_rpm_0_1;
	case RightEngineCoreRelatedRpm: return display.right_core_related_rpm_0_1;
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
			? Common::limit(
				state.right_engine_power_readout_normalized *
					kEngineCombustionScale,
				0.0,
				1.0)
			: 0.0;
	case RightEngineRelatedThrust:
	case RightEngineCoreRelatedThrust:
		return state.right_throttle_output_normalized;
	case RightEngineCoreThrust:
	case RightEngineThrust: return state.right_thrust_force_n;
	case RightEngineTemperature:
		return engine_temperature_c(
			state.right_engine_power_readout_normalized,
			state.atmosphere_temperature_k);
	default: return std::nullopt;
	}
}

inline std::optional<double> get_param(
	unsigned index,
	const ParamExportState& state)
{
	std::optional<double> result = lookup_engine_compatibility(index, state);
	if (result) return result;
	result = lookup_system_compatibility(index);
	if (result) return result;
	result = lookup_wheel_motion(index, state);
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

enum class ParamExportStatus
{
	Value,
	StartCompatibility,
	Unknown,
	MissingRuntimeData
};

struct ParamExportResult
{
	ParamExportStatus status = ParamExportStatus::Unknown;
	double value = 0.0;
	std::optional<ParamDataCategory> missing_data;
};

struct ParamExportAvailabilityHistory
{
	bool atmosphere_available = false;
	bool nose_suspension_available = false;
};

inline void observe_param_export_availability(
	ParamExportAvailabilityHistory& history,
	const Core::FrameDataAvailability& availability)
{
	history.atmosphere_available =
		history.atmosphere_available || availability.atmosphere;
	history.nose_suspension_available =
		history.nose_suspension_available || availability.suspension[0];
}

inline bool was_param_data_available(
	const ParamExportAvailabilityHistory& history,
	ParamDataCategory category)
{
	switch (category)
	{
	case ParamDataCategory::Atmosphere:
		return history.atmosphere_available;
	case ParamDataCategory::Suspension:
		return history.nose_suspension_available;
	default: return false;
	}
}

inline std::optional<double> lookup_start_compatibility(unsigned index)
{
	using namespace DcsIds::Params;
	switch (index)
	{
	case NoseWheelYaw:
	case LeftEngineTemperature:
	case RightEngineTemperature:
		return 0.0;
	default: return std::nullopt;
	}
}

inline ParamExportResult resolve_param(
	unsigned index,
	const Core::FrameOutput& output,
	const ParamExportAvailabilityHistory& history)
{
	const ParamExportState state = make_param_export_state(output);
	const std::optional<ParamDataCategory> missing =
		missing_param_data(index, state);
	if (missing)
	{
		const std::optional<double> compatibility =
			lookup_start_compatibility(index);
		if (compatibility && !was_param_data_available(history, *missing))
		{
			return {
				ParamExportStatus::StartCompatibility,
				*compatibility,
				missing
			};
		}
		return { ParamExportStatus::MissingRuntimeData, 0.0, missing };
	}
	const std::optional<double> value = get_param(index, state);
	return value
		? ParamExportResult{ ParamExportStatus::Value, *value }
		: ParamExportResult{};
}
}
