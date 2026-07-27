#pragma once

#include "Core/Fck1cEfm.h"

namespace Tests
{
namespace Fck1c
{
inline Data::AircraftConfig make_test_config()
{
	Data::AircraftConfig config;
	config.aerodynamics.wing_area = 24.26;
	config.aerodynamics.wingspan = 8.53;
	config.aerodynamics.length = 14.48;
	config.aerodynamics.height = 4.7;
	config.aerodynamics.mach_max = 1.5;
	config.aerodynamics.mach_table = { 0.0, 1.0 };
	config.aerodynamics.cx_zero_table = { 0.025, 0.030 };
	config.aerodynamics.cy_alpha_table = { 0.05, 0.04 };
	config.aerodynamics.roll_rate_max_table = { 3.0, 2.0 };
	config.aerodynamics.alpha_max_table = { 20.0, 18.0 };
	config.aerodynamics.cy_max_table = { 1.2, 1.0 };
	config.engine.start_time = 5.0;
	config.engine.spool_up_tau = 1.0;
	config.engine.spool_down_tau = 1.0;
	config.engine.mach_table = { 0.0, 1.0 };
	config.engine.max_thrust_table = { 54000.0, 50000.0 };
	config.engine.throttle_input_table = { 0.0, 1.0 };
	config.engine.power_table = { 0.1, 1.0 };
	config.left_engine_position = { -3.793, -0.391, -0.716 };
	config.right_engine_position = { -3.793, -0.391, 0.716 };
	return config;
}

inline Core::FrameDataAvailability all_frame_data_available()
{
	return { true, true, true, true, true, { true, true, true } };
}

inline Core::FrameInput make_frame_input()
{
	Core::FrameInput input;
	input.dt_s = 0.02;
	input.availability = all_frame_data_available();
	input.atmosphere = {
		1200.0, 281.0, 330.0, 1.1, 88000.0, { 5.0, 1.0, -2.0 }
	};
	input.surface = { 200.0, 203.0, 4, { 0.0, 1.0, 0.0 } };
	input.mass = { 9400.0, { 0.2, -0.1, 0.3 }, { 11.0, 12.0, 13.0 } };
	input.world_kinematics = {
		{ 0.1, 0.2, 0.3 }, { 150.0, 4.0, 2.0 }, { 10.0, 20.0, 1200.0 },
		{ 0.01, 0.02, 0.03 }, { 0.1, 0.2, 0.3 }, { 0.0, 0.0, 0.0, 1.0 }
	};
	input.body_kinematics = {
		{ 0.0, 9.81, 0.0 }, { 140.0, 3.0, 1.0 }, { 4.0, 0.5, -1.0 },
		{ 0.02, 0.03, 0.04 }, { 0.05, 0.06, 0.07 },
		0.3, 0.1, -0.2, 0.15, -0.04
	};
	input.suspension = {
		Core::SuspensionFeedbackInput{
			0, { 3.0, 4.0, 0.0 }, { 1.0, 2.0, 3.0 }, 0.9, 0.10, 12.0 },
		Core::SuspensionFeedbackInput{
			1, { 0.0, 80.0, 0.0 }, { 4.0, 5.0, 6.0 }, 0.8, 0.20, 13.0 },
		Core::SuspensionFeedbackInput{
			2, { 0.0, 90.0, 0.0 }, { 7.0, 8.0, 9.0 }, 0.7, 0.30, 14.0 }
	};
	input.autopilot = { true, false, true, 0.2, -0.3, 0.4 };
	input.max_power = { 1.0, 1.0 };
	return input;
}
}
}
