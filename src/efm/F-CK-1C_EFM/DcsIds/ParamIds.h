#pragma once

#include "../include/FM/wHumanCustomPhysicsAPI.h"

namespace DcsIds
{
namespace Params
{
static constexpr unsigned NoseWheelYaw = ED_FM_SUSPENSION_0_WHEEL_YAW;
static constexpr unsigned NoseWheelSpin = ED_FM_SUSPENSION_0_WHEEL_SELF_ATTITUDE;
static constexpr unsigned LeftWheelSpin = ED_FM_SUSPENSION_1_WHEEL_SELF_ATTITUDE;
static constexpr unsigned RightWheelSpin = ED_FM_SUSPENSION_2_WHEEL_SELF_ATTITUDE;
static constexpr unsigned NoseBrakeMoment = ED_FM_SUSPENSION_0_RELATIVE_BRAKE_MOMENT;
static constexpr unsigned LeftBrakeMoment = ED_FM_SUSPENSION_1_RELATIVE_BRAKE_MOMENT;
static constexpr unsigned RightBrakeMoment = ED_FM_SUSPENSION_2_RELATIVE_BRAKE_MOMENT;
static constexpr unsigned AntiSkidEnable = ED_FM_ANTI_SKID_ENABLE;

static constexpr unsigned StickPitch = ED_FM_FC3_STICK_PITCH;
static constexpr unsigned StickRoll = ED_FM_FC3_STICK_ROLL;
static constexpr unsigned RudderPedals = ED_FM_FC3_RUDDER_PEDALS;
static constexpr unsigned WheelBrakeLeft = ED_FM_FC3_WHEEL_BRAKE_LEFT;
static constexpr unsigned WheelBrakeCommandLeft = ED_FM_FC3_WHEEL_BRAKE_COMMAND_LEFT;
static constexpr unsigned WheelBrakeRight = ED_FM_FC3_WHEEL_BRAKE_RIGHT;
static constexpr unsigned WheelBrakeCommandRight = ED_FM_FC3_WHEEL_BRAKE_COMMAND_RIGHT;
static constexpr unsigned ThrottleLeft = ED_FM_FC3_THROTTLE_LEFT;
static constexpr unsigned ThrottleRight = ED_FM_FC3_THROTTLE_RIGHT;

static constexpr unsigned InternalFuel = ED_FM_FUEL_INTERNAL_FUEL;
static constexpr unsigned TotalFuel = ED_FM_FUEL_TOTAL_FUEL;
static constexpr unsigned OxygenSupply = ED_FM_OXYGEN_SUPPLY;
static constexpr unsigned FlowVelocity = ED_FM_FLOW_VELOCITY;

static constexpr unsigned NoseGearPostState = ED_FM_SUSPENSION_0_GEAR_POST_STATE;
static constexpr unsigned LeftGearPostState = ED_FM_SUSPENSION_1_GEAR_POST_STATE;
static constexpr unsigned RightGearPostState = ED_FM_SUSPENSION_2_GEAR_POST_STATE;

static constexpr unsigned ApuRpm = ED_FM_ENGINE_0_RPM;
static constexpr unsigned ApuRelatedRpm = ED_FM_ENGINE_0_RELATED_RPM;
static constexpr unsigned ApuCoreRelatedRpm = ED_FM_ENGINE_0_CORE_RELATED_RPM;
static constexpr unsigned ApuThrust = ED_FM_ENGINE_0_THRUST;
static constexpr unsigned ApuRelatedThrust = ED_FM_ENGINE_0_RELATED_THRUST;

static constexpr unsigned LeftPropellerPitch = ED_FM_PROPELLER_1_PITCH;
static constexpr unsigned LeftEngineFuelFlow = ED_FM_ENGINE_1_FUEL_FLOW;
static constexpr unsigned LeftEngineFanPhase = ED_FM_ENGINE_1_FAN_PHASE;
// The bundled SDK leaves 131 unnamed.
static constexpr unsigned LeftEngineFlowSpeedCompatibility = 131;
static constexpr unsigned LeftEngineCoreRpm = ED_FM_ENGINE_1_CORE_RPM;
static constexpr unsigned LeftEngineRpm = ED_FM_ENGINE_1_RPM;
static constexpr unsigned LeftEngineCombustion = ED_FM_ENGINE_1_COMBUSTION;
static constexpr unsigned LeftEngineRelatedThrust = ED_FM_ENGINE_1_RELATED_THRUST;
static constexpr unsigned LeftEngineCoreRelatedThrust = ED_FM_ENGINE_1_CORE_RELATED_THRUST;
static constexpr unsigned LeftEngineRelatedRpm = ED_FM_ENGINE_1_RELATED_RPM;
static constexpr unsigned LeftEngineCoreRelatedRpm = ED_FM_ENGINE_1_CORE_RELATED_RPM;
static constexpr unsigned LeftEngineCoreThrust = ED_FM_ENGINE_1_CORE_THRUST;
static constexpr unsigned LeftEngineThrust = ED_FM_ENGINE_1_THRUST;
static constexpr unsigned LeftEngineTemperature = ED_FM_ENGINE_1_TEMPERATURE;

static constexpr unsigned RightPropellerPitch = ED_FM_PROPELLER_2_PITCH;
static constexpr unsigned RightEngineFuelFlow = ED_FM_ENGINE_2_FUEL_FLOW;
static constexpr unsigned RightEngineFanPhase = ED_FM_ENGINE_2_FAN_PHASE;
// The bundled SDK leaves 231 unnamed.
static constexpr unsigned RightEngineFlowSpeedCompatibility = 231;
static constexpr unsigned RightEngineCoreRpm = ED_FM_ENGINE_2_CORE_RPM;
static constexpr unsigned RightEngineRpm = ED_FM_ENGINE_2_RPM;
static constexpr unsigned RightEngineCombustion = ED_FM_ENGINE_2_COMBUSTION;
static constexpr unsigned RightEngineRelatedThrust = ED_FM_ENGINE_2_RELATED_THRUST;
static constexpr unsigned RightEngineCoreRelatedThrust = ED_FM_ENGINE_2_CORE_RELATED_THRUST;
static constexpr unsigned RightEngineRelatedRpm = ED_FM_ENGINE_2_RELATED_RPM;
static constexpr unsigned RightEngineCoreRelatedRpm = ED_FM_ENGINE_2_CORE_RELATED_RPM;
static constexpr unsigned RightEngineCoreThrust = ED_FM_ENGINE_2_CORE_THRUST;
static constexpr unsigned RightEngineThrust = ED_FM_ENGINE_2_THRUST;
static constexpr unsigned RightEngineTemperature = ED_FM_ENGINE_2_TEMPERATURE;

static constexpr unsigned PitchForceFactor = ED_FM_STICK_FORCE_FACTOR_PITCH;
static constexpr unsigned PitchForceShakeAmplitude =
	ED_FM_STICK_FORCE_SHAKE_AMPLITUDE_PITCH;
static constexpr unsigned PitchForceShakeFrequency =
	ED_FM_STICK_FORCE_SHAKE_FREQUENCY_PITCH;
static constexpr unsigned RollForceCenter = ED_FM_STICK_FORCE_CENTRAL_ROLL;
static constexpr unsigned RollForceFactor = ED_FM_STICK_FORCE_FACTOR_ROLL;
static constexpr unsigned RollForceShakeAmplitude =
	ED_FM_STICK_FORCE_SHAKE_AMPLITUDE_ROLL;
static constexpr unsigned RollForceShakeFrequency =
	ED_FM_STICK_FORCE_SHAKE_FREQUENCY_ROLL;
static constexpr unsigned CockpitPressurization =
	ED_FM_COCKPIT_PRESSURIZATION_OVER_EXTERNAL;
static constexpr unsigned InterruptRefuel = ED_FM_INTERRUPT_REFUEL;

// DCS queried these indices, but the bundled SDK does not define their meaning.
static constexpr unsigned UnknownCompatibility2134 = 2134;
static constexpr unsigned UnknownCompatibility2135 = 2135;
static constexpr unsigned UnknownCompatibility2136 = 2136;
static constexpr unsigned UnknownCompatibility2137 = 2137;
}
}
