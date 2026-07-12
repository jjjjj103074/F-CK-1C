#pragma once
#include "stdafx.h"

namespace FM_DATA
{
	static const unsigned kAeroTableSize = 6;
	static const unsigned kEngineTableSize = 11;

	// EFMSTATE: Data/AircraftConfig - aircraft geometry constants; move to immutable config.
	extern double wing_area; // Wing area (sq. m)
	extern double wingspan;  // Wing span (m)
	extern double length;    // Length (m)
	extern double height;    // Height (m)
	extern double mach_max;  // Max airspeed (mach)

	// Mass, weight, and inertia are supplied by the aircraft definition.

	// EFMSTATE: Data/AeroTables - aerodynamic constants and lookup tables.
	extern double Cy0; // Zero AoA lift coefficient
	extern double Czbe; // Yaw/side force coefficient
	extern double cx_gear; // Gear drag
	extern double cx_brk; // Air brake drag
	extern double cx_flap; // Flap drag
	extern double cx_lift_k; // Additional drag from lift production
	extern double cx_alpha_k; // Additional drag from AoA in radians
	extern double cx_elevator_k; // Additional drag from large elevator deflection
	extern double cy_flap; // Flap lift

	extern double mach_table[kAeroTableSize];
	extern double cx0[kAeroTableSize];
	extern double Cya[kAeroTableSize];
	extern double OmxMax[kAeroTableSize];
	extern double Aldop[kAeroTableSize];
	extern double CyMax[kAeroTableSize];

	// EFMSTATE: Data/EngineTables - engine constants and lookup tables.
	extern double idle_rpm; // RPM % at idle
	extern double fuel_consumption; // Fuel consumption at full throttle (Kg/s)
	extern double engine_start_time; // Engine startup time (s)
	extern double engine_spool_up_tau; // Time constant for throttle increase (idle->MIL)
	extern double engine_spool_down_tau; // Time constant for throttle decrease (MIL->idle)
	extern double airbrake_pitch_comp_k; // Speedbrake pitch compensation coefficient

	extern double engine_mach_table[kEngineTableSize];
	extern double max_thrust[kEngineTableSize];
	extern double throttle_input_table[kEngineTableSize];
	extern double engine_power_table[kEngineTableSize];
	extern double engine_power_readout_table[kEngineTableSize];
}
