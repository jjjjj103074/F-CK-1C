#include "AeroTables.h"

namespace
{
Data::AeroTables make_fck1c_aero_tables()
{
	Data::AeroTables tables;
	tables.cy_zero = 0.0001;
	tables.cz_beta = -0.016;
	tables.cx_gear = 0.012;
	tables.cx_airbrake = 0.06;
	tables.cx_flap = 0.05;
	tables.cx_lift_k = 0.030;
	tables.cx_alpha_k = 0.080;
	tables.cx_elevator_k = 0.008;
	tables.cy_flap = 0.3;
	tables.airbrake_pitch_comp_k = 0.003;
	tables.mach = { 0.0, 0.4, 0.6, 0.8, 0.9, 1.5 };
	tables.cx_zero = { 0.025, 0.025, 0.0272, 0.048, 0.0741, 0.0741 };
	tables.cy_alpha = { 0.0817, 0.0817, 0.0872, 0.0816, 0.08, 0.08 };
	tables.roll_rate_max = { 0.5, 1.5, 2.5, 3.5, 3.5, 3.5 };
	tables.alpha_max = { 20.0, 20.0, 20.0, 18.0, 15.0, 10.0 };
	tables.cy_max = { 1.21, 1.21, 1.26, 0.755, 0.6, 0.6 };
	return tables;
}
}

namespace Data
{
const AeroTables& fck1c_aero_tables()
{
	static const AeroTables tables = make_fck1c_aero_tables();
	return tables;
}
}
