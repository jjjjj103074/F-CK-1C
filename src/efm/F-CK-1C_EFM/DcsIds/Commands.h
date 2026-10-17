#pragma once

#include "CustomCommands.g.h"

namespace DcsIds
{
namespace Commands
{
// DCS built-in command IDs. Their values are part of the DCS input contract.
static constexpr int JoystickPitch = 2001;
static constexpr int PitchUp = 195;
static constexpr int PitchUpStop = 196;
static constexpr int PitchDown = 193;
static constexpr int PitchDownStop = 194;
static constexpr int TrimUp = 95;
static constexpr int TrimDown = 96;

static constexpr int JoystickRoll = 2002;
static constexpr int RollLeft = 197;
static constexpr int RollLeftStop = 198;
static constexpr int RollRight = 199;
static constexpr int RollRightStop = 200;
static constexpr int TrimLeft = 93;
static constexpr int TrimRight = 94;

static constexpr int PedalYaw = 2003;
static constexpr int RudderLeft = 201;
static constexpr int RudderLeftStop = 202;
static constexpr int RudderRight = 203;
static constexpr int RudderRightStop = 204;
static constexpr int RudderTrimLeft = 98;
static constexpr int RudderTrimRight = 99;

static constexpr int EnginesOn = 309;
static constexpr int LeftEngineOn = 311;
static constexpr int RightEngineOn = 312;
static constexpr int EnginesOff = 310;
static constexpr int LeftEngineOff = 313;
static constexpr int RightEngineOff = 314;

static constexpr int ThrottleAxis = 2004;
static constexpr int ThrottleAxisLeft = 2005;
static constexpr int ThrottleAxisRight = 2006;
static constexpr int ThrottleLeftUp = 161;
static constexpr int ThrottleRightUp = 163;
static constexpr int ThrottleLeftDown = 162;
static constexpr int ThrottleRightDown = 164;
static constexpr int ThrottleIncrease = 1032;
static constexpr int ThrottleDecrease = 1033;
static constexpr int ThrottleStop = 1034;

static constexpr int GearToggle = 68;
static constexpr int GearUp = 430;
static constexpr int GearDown = 431;
static constexpr int WheelBrakeOn = 74;
static constexpr int WheelBrakeOff = 75;
static constexpr int WheelBrakeLeftOn = 961;
static constexpr int WheelBrakeLeftOff = 962;
static constexpr int WheelBrakeRightOn = 963;
static constexpr int WheelBrakeRightOff = 964;

static constexpr int AirBrakes = 73;
static constexpr int AirBrakesOn = 147;
static constexpr int AirBrakesOff = 148;

static constexpr int FlapsToggle = 72;
static constexpr int FlapsUp = 145;
static constexpr int FlapsDown = 146;
static constexpr int ResetTrim = 97;
static constexpr int PlaneFire = 84;
static constexpr int PlaneFireOff = 85;
static constexpr int PlaneDropFlareOnce = 357;
}
}
