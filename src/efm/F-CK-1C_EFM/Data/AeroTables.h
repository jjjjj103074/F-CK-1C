#pragma once

#include <array>
#include <cstddef>

namespace Data
{
constexpr std::size_t kAeroTableSize = 6;
using AeroTable = std::array<double, kAeroTableSize>;

struct AeroTables
{
	double cy_zero = 0.0;
	double cz_beta = 0.0;
	double cx_gear = 0.0;
	double cx_airbrake = 0.0;
	double cx_flap = 0.0;
	double cx_lift_k = 0.0;
	double cx_alpha_k = 0.0;
	double cx_elevator_k = 0.0;
	double cy_flap = 0.0;
	double airbrake_pitch_comp_k = 0.0;
	AeroTable mach;
	AeroTable cx_zero;
	AeroTable cy_alpha;
	AeroTable roll_rate_max;
	AeroTable alpha_max;
	AeroTable cy_max;
};

const AeroTables& fck1c_aero_tables();
}
