#pragma once

#include "../Common/Actuator.h"
#include "../Common/Clamp.h"

namespace Systems
{
struct AirframeDeviceState
{
	bool airbrake_switch = false;
	double airbrake_pos = 0.0;
	int flap_mode = 0;
	double flaps_pos = 0.0;
	double slats_pos = 0.0;
	bool gear_switch = false;
	double gear_pos = 0.0;
};

inline void toggle_airbrake(AirframeDeviceState& devices)
{
	devices.airbrake_switch = !devices.airbrake_switch;
}

inline void set_airbrake(AirframeDeviceState& devices, bool enabled)
{
	devices.airbrake_switch = enabled;
}

inline void toggle_flap_mode(AirframeDeviceState& devices, int up_mode, int down_mode)
{
	devices.flap_mode = (devices.flap_mode == down_mode) ? up_mode : down_mode;
}

inline void set_flap_mode(AirframeDeviceState& devices, int mode)
{
	devices.flap_mode = mode;
}

inline void toggle_gear(AirframeDeviceState& devices)
{
	devices.gear_switch = !devices.gear_switch;
}

inline void set_gear(AirframeDeviceState& devices, bool down)
{
	devices.gear_switch = down;
}

inline void configure_ground_start_devices(AirframeDeviceState& devices)
{
	devices.gear_switch = true;
	devices.gear_pos = 1.0;
}

inline void configure_hot_ground_start_devices(AirframeDeviceState& devices, int down_mode)
{
	configure_ground_start_devices(devices);
	devices.flap_mode = down_mode;
	devices.flaps_pos = 1.0;
	devices.slats_pos = 1.0;
}

inline void configure_air_start_devices(AirframeDeviceState& devices)
{
	devices.gear_switch = false;
	devices.gear_pos = 0.0;
}

inline double compute_flap_target(
	const AirframeDeviceState& devices,
	double v_scalar,
	int down_mode,
	int auto_mode)
{
	if (devices.flap_mode == down_mode)
	{
		return 1.0;
	}

	if (devices.flap_mode == auto_mode)
	{
		const double ias_kts = v_scalar * 1.943844;
		if (devices.gear_pos > 0.5 || ias_kts <= 240.0)
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
	double v_scalar,
	int down_mode,
	int auto_mode)
{
	devices.gear_pos = Common::limit(
		Common::actuator(devices.gear_pos, devices.gear_switch, -0.001, 0.001),
		0.0,
		1.0);
	devices.airbrake_pos = Common::limit(
		Common::actuator(devices.airbrake_pos, devices.airbrake_switch, -0.003, 0.004),
		0.0,
		1.0);

	const double flap_target = compute_flap_target(devices, v_scalar, down_mode, auto_mode);
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
