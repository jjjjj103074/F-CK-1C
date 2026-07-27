#pragma once

#include <vector>

namespace Core
{
namespace Simulation
{
struct AerodynamicsConfig
{
	double wing_area = 0.0;
	double wingspan = 0.0;
	double length = 0.0;
	double height = 0.0;
	double mach_max = 0.0;

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

	std::vector<double> mach_table;
	std::vector<double> cx_zero_table;
	std::vector<double> cy_alpha_table;
	std::vector<double> roll_rate_max_table;
	std::vector<double> alpha_max_table;
	std::vector<double> cy_max_table;
};

void validate_aerodynamics_config(const AerodynamicsConfig& config);
const AerodynamicsConfig& fck1c_aerodynamics_config();
}
}
