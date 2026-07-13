#pragma once

#include "../Common/Actuator.h"
#include "../Common/Clamp.h"

namespace Systems
{
enum FlapMode
{
	FLAP_MODE_UP = 0,
	FLAP_MODE_AUTO = 1,
	FLAP_MODE_DOWN = 2
};

struct AirframeDeviceState
{
	bool airbrake_switch = false;
	double airbrake_pos = 0.0;
	FlapMode flap_mode = FLAP_MODE_UP;
	double flaps_pos = 0.0;
	double slats_pos = 0.0;
};

struct AirframeDeviceUpdateInput
{
	double speed_scalar = 0.0;
	double gear_position = 0.0;
};

inline void toggle_airbrake(AirframeDeviceState& devices)
{
	devices.airbrake_switch = !devices.airbrake_switch;
}

inline void set_airbrake(AirframeDeviceState& devices, bool enabled)
{
	devices.airbrake_switch = enabled;
}

inline void toggle_flap_mode(AirframeDeviceState& devices)
{
	devices.flap_mode = (devices.flap_mode == FLAP_MODE_DOWN)
		? FLAP_MODE_UP
		: FLAP_MODE_DOWN;
}

inline void set_flap_mode(AirframeDeviceState& devices, FlapMode mode)
{
	devices.flap_mode = mode;
}

inline void configure_hot_ground_start_devices(AirframeDeviceState& devices)
{
	devices.flap_mode = FLAP_MODE_DOWN;
	devices.flaps_pos = 1.0;
	devices.slats_pos = 1.0;
}

inline double compute_flap_target(
	const AirframeDeviceState& devices,
	const AirframeDeviceUpdateInput& input)
{
	if (devices.flap_mode == FLAP_MODE_DOWN)
	{
		return 1.0;
	}

	if (devices.flap_mode == FLAP_MODE_AUTO)
	{
		const double ias_kts = input.speed_scalar * 1.943844;
		if (input.gear_position > 0.5 || ias_kts <= 240.0)
		{
			return 1.0;
		}
		if (ias_kts >= 450.0)
		{
			return 0.0;
		}
		return 1.0 - ((ias_kts - 240.0) / (450.0 - 240.0));
	}

	return 0.0;
}

inline void update_airframe_device_positions(
	AirframeDeviceState& devices,
	const AirframeDeviceUpdateInput& input)
{
	devices.airbrake_pos = Common::limit(
		Common::actuator(devices.airbrake_pos, devices.airbrake_switch, -0.003, 0.004),
		0.0,
		1.0);

	const double flap_target = compute_flap_target(devices, input);
	devices.flaps_pos = Common::limit(
		Common::actuator(devices.flaps_pos, flap_target, -0.002, 0.002),
		0.0,
		1.0);
	devices.slats_pos = Common::limit(
		Common::actuator(devices.slats_pos, flap_target, -0.003, 0.003),
		0.0,
		1.0);
}
}
