#pragma once

#include "../../../../Common/Vec3.h"

#include <vector>

namespace Core
{
namespace Simulation
{
struct PropulsionConfig
{
	std::vector<double> mach_table;
	std::vector<double> max_thrust_table_n;
	double afterburner_thrust_factor = 1.0;
	Common::Vec3 left_engine_position_body_m;
	Common::Vec3 right_engine_position_body_m;
};

void validate_propulsion_config(const PropulsionConfig& config);
const PropulsionConfig& fck1c_propulsion_config();
double fck1c_carrier_launch_reference_thrust_n();
}
}
