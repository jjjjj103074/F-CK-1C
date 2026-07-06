#pragma once

enum InputCommands
{
	// Command IDs from Lua command_defs.lua and DCS built-in bindings.

	// Pitch
	JoystickPitch = 2001,
	PitchUp = 195,
	PitchUpStop = 196,
	PitchDown = 193,
	PitchDownStop = 194,
	trimUp = 95,
	trimDown = 96,

	// Roll
	JoystickRoll = 2002,
	RollLeft = 197,
	RollLeftStop = 198,
	RollRight = 199,
	RollRightStop = 200,
	trimLeft = 93,
	trimRight = 94,

	// Yaw
	PedalYaw = 2003,
	rudderleft = 201,
	rudderleftstop = 202,
	rudderright = 203,
	rudderrightstop = 204,
	ruddertrimLeft = 98,
	ruddertrimRight = 99,

	// Engine and throttle commands
	EnginesOn = 309,
	LeftEngineOn = 311,
	RightEngineOn = 312,

	EnginesOff = 310,
	LeftEngineOff = 313,
	RightEngineOff = 314,

	ThrottleAxis = 2004, // Both engines
	ThrottleAxisLeft = 2005,
	ThrottleAxisRight = 2006,
	ThrottleLeftUp = 161,
	ThrottleRightUp = 163,
	ThrottleLeftDown = 162,
	ThrottleRightDown = 164,
	ThrottleIncrease = 1032,
	ThrottleDecrease = 1033,
	ThrottleStop = 1034,

	// Gear commands
	gearToggle = 68,
	gearUp = 430,
	gearDown = 431,
	WheelBrakeOn = 74,
	WheelBrakeOff = 75,
	WheelBrakeLeftOn = 961,
	WheelBrakeLeftOff = 962,
	WheelBrakeRightOn = 963,
	WheelBrakeRightOff = 964,

	// Air brake commands
	AirBrakes = 73,
	AirBrakesOn = 147,
	AirBrakesOff = 148,

	// Flap commands
	flapsToggle = 72,
	flapsUp = 145,
	flapsDown = 146,

	// Misc controls
	resetTrim = 97,

	// FBW controls (custom)
	FBWCatToggle = 3007,
	FBWCat1 = 3008,
	FBWCat3 = 3009,
	AirBrakesAuto = 3010,
	GearAuto = 3011,
	FlapsAuto = 3012,
	NoseTurnUp = 3013,
	NoseTurnAuto = 3014,
	NoseTurnDown = 3015,
	NoseTurnToggle = 3022,
	WheelBrakeAxis = 3023,
	WheelBrakeAxisLeft = 3024,
	WheelBrakeAxisRight = 3025,
	FBWGLimiterOverride = 3026,
	FBWGLimiterOverrideToggle = 3027,
	AirBrakesUp = 3016,
	AirBrakesDown = 3017,
	GearHandleUp = 3018,
	GearHandleDown = 3019,
	FlapsUpCmd = 3020,
	FlapsDownCmd = 3021,
	TriggerFirstStage = 3030,
	CMSForward = 3031,
	CMSAft = 3032,
	CMSLeft = 3033,
	CMSRight = 3034,
	CMSPress = 3035,
	TriggerSecondStage = 3036,
	MasterArmOn = 3040,
	MasterArmOff = 3041,
	MasterArmSim = 3042,
	DogfightSwitch = 3043,
	PlaneFire = 84,
	PlaneFireOff = 85,
	PlaneDropFlareOnce = 357,

	// Autopilot commands (routed to Lua device, listed here for reference)
	APMasterToggle = 3100,
	APMasterOn = 3101,
	APMasterOff = 3102,
	APBypass = 3103,
	APVertPitchHold = 3110,
	APVertVSHold = 3111,
	APVertAltHold = 3112,
	APVertIncrease = 3113,
	APVertDecrease = 3114,
	APLatHeadingHold = 3120,
	APLatHeadingSelect = 3121,
	APLatNavTrack = 3122,
	APLatIncrease = 3123,
	APLatDecrease = 3124,
	APAutoThrottleToggle = 3130,
	APAutoThrottleOn = 3131,
	APAutoThrottleOff = 3132,
	APSpeedIncrease = 3133,
	APSpeedDecrease = 3134,

	Reserved
};
