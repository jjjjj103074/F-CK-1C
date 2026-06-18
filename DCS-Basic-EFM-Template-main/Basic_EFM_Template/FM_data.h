#pragma once
#include "stdafx.h"

namespace FM_DATA
{
	// Default data here is from the Su-25T with some slight modifications


	double wing_area = 24.26; // Wing area (sq. m)
	double wingspan = 8.53;// Wing span (m)
	double length = 14.48;// Length (m)
	double height = 4.7;// Height (m)
	double mach_max = 1.8; // Max airspeed (mach)

	// Mass, weight, inertia, and moment of inertia are all calculated automatically from the aircraft definition file.

	double Cy0 = 0.0001; // Zero AoA lift coefficient
	double Czbe = -0.016; // Yaw/side force coefficient

	double cx_gear = 0.012; // Gear drag

	double cx_brk = 0.06; // Air brake drag

	double cx_flap = 0.05; // Flap drag
	double cx_lift_k = 0.030; // Additional drag from lift production
	double cx_alpha_k = 0.080; // Additional drag from AoA in radians
	double cx_elevator_k = 0.008; // Additional drag from large elevator deflection
	double cy_flap = 0.3; // Flap lift

	// Important: Make sure the first value is 0.
	// Make sure the number of entries (size) of all other tables are exactly the same.
	double mach_table[] =
	{
		0,
		0.4,
		0.6,
		0.8,
		0.9,
		1.5,
	};

	double cx0[] = // Drag coefficient
	{
		0.025, // m = 0
		0.025, // m = 0.4
		0.0272, // m = 0.6
		0.048, // m = 0.8
		0.0741, // m = 0.9
		0.0741, // m = 1.5
	};

	double Cya[] = // Lift coefficient
	{
		0.0817,
		0.0817,
		0.0872,
		0.0816,
		0.08,
		0.08,
	};
	
	/* 
	In the SFM and the template, there are tables for "B" and "B4", 
	those being drag polar and drag polar quad coefficients.

	Implementing these into the drag force leads to far worse performance 
	(greater deceleration) compared to official DCS modules, so they are excluded here.
	*/

	double OmxMax[] = // Max roll rate, radians per second
	{
		0.5,
		1.5,
		2.5,
		3.5,
		3.5,
		3.5,
	};

	double Aldop[] = // Max Alpha, degrees
	{
		20,
		20,
		20,
		18,
		15,
		10,
	};

	double CyMax[] =  // Max lift coefficient
	{
		1.21,
		1.21,
		1.26,
		0.755,
		0.6,
		0.6,
	};


	// Engine(s)
	double idle_rpm = 60.0; // RPM % at idle
	double fuel_consumption = 0.37; // Fuel consumption at full throttle (Kg/s)
	double engine_start_time = 60; // Engine startup time (s)

	// TFE1042-70 throttle spool lag (first-order time constants, seconds).
	// Spool-up ~3s and spool-down ~5s are consistent with medium-bypass turbofan behaviour.
	// These apply to left_throttle_output / right_throttle_output tracking their target values.
	double engine_spool_up_tau   = 2.5; // Time constant for throttle increase (idle->MIL)
	double engine_spool_down_tau = 4.0; // Time constant for throttle decrease (MIL->idle)

	// Speedbrake pitch compensation coefficient.
	// F-CK-1C FLCS couples airbrake deployment to a nose-up pitch trim feedforward to
	// prevent the pitch-down disturbance that occurs when the speedbrake is opened at speed.
	// Units: Cm / (airbrake_pos [0-1] * q_bar [Pa]) -- applied as Moment = k * ab * q * S * c_bar
	double airbrake_pitch_comp_k = 0.003;

	// Important: Make sure the first value is 0.
	// Make sure the number of entries (size) of all other tables are exactly the same.
	double engine_mach_table[] =
	{
		0,
		0.1,
		0.2,
		0.3,
		0.4,
		0.5,
		0.6,
		0.7,
		0.8,
		0.9,
		1.0,
	};

	// Max total thrust, Newtons.
	double max_thrust[] =
	{
		54000.0,
		53600.0,
		53200.0,
		52800.0,
		52300.0,
		51600.0,
		50800.0,
		49900.0,
		48900.0,
		47800.0,
		46600.0,
	};
	// This is total thrust, not thrust per engine if there's more than one.

	// Throttle to thrust response curves

	// Throttle level
	double throttle_input_table[] =
	{
		0,
		0.1,
		0.2,
		0.3,
		0.4,
		0.5,
		0.6,
		0.7,
		0.8,
		0.9,
		1.0,
	};

	// Throttle output
	double engine_power_table[] =
	{
		0,		// 0
		0.01,	// 0.1
		0.02,	// 0.2
		0.06,	// 0.3
		0.08,	// 0.4
		0.1,	// 0.5
		0.3,	// 0.6
		0.5,	// 0.7
		0.7,	// 0.8
		0.9,	// 0.9
		1.0,	// 1.0
	};

	// RPM or engine power readout
	double engine_power_readout_table[] =
	{
		0.6,	// 0
		0.64,	// 0.1
		0.68,	// 0.2
		0.72,	// 0.3
		0.76,	// 0.4
		0.8,	// 0.5
		0.84,	// 0.6
		0.88,	// 0.7
		0.92,	// 0.8
		0.96,	// 0.9
		1.0,	// 1.0
	};
}
