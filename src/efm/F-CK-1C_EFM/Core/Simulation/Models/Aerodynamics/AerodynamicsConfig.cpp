#include "AerodynamicsConfig.h"

#include "../../../../Common/ConfigValidation.h"

#include <algorithm>
#include <stdexcept>

namespace
{
Core::Simulation::AerodynamicsConfig make_fck1c_config()
{
	Core::Simulation::AerodynamicsConfig config;
	config.wing_area = 24.26;
	config.wingspan = 8.53;
	config.length = 14.48;
	config.height = 4.7;
	config.mach_max = 1.8;
	config.cy_zero = 0.0001;
	config.cz_beta = -0.016;
	config.cx_gear = 0.012;
	config.cx_airbrake = 0.06;
	config.cx_flap = 0.05;
	config.cx_lift_k = 0.030;
	config.cx_alpha_k = 0.080;
	config.cx_elevator_k = 0.008;
	config.cy_flap = 0.3;
	config.airbrake_pitch_comp_k = 0.003;
	config.mach_table = { 0.0, 0.4, 0.6, 0.8, 0.9, 1.5 };
	config.cx_zero_table = {
		0.025, 0.025, 0.0272, 0.048, 0.0741, 0.0741
	};
	config.cy_alpha_table = {
		0.0817, 0.0817, 0.0872, 0.0816, 0.08, 0.08
	};
	config.roll_rate_max_table = { 0.5, 1.5, 2.5, 3.5, 3.5, 3.5 };
	config.alpha_max_table = { 20.0, 20.0, 20.0, 18.0, 15.0, 10.0 };
	config.cy_max_table = { 1.21, 1.21, 1.26, 0.755, 0.6, 0.6 };
	return config;
}

bool valid_geometry(const Core::Simulation::AerodynamicsConfig& config)
{
	return config.wing_area > 0.0 && config.wingspan > 0.0 &&
		config.length > 0.0 && config.height > 0.0 &&
		config.mach_max > 0.0;
}

bool matching_table_sizes(
	const Core::Simulation::AerodynamicsConfig& config)
{
	const std::size_t size = config.mach_table.size();
	return config.cx_zero_table.size() == size &&
		config.cy_alpha_table.size() == size &&
		config.roll_rate_max_table.size() == size &&
		config.alpha_max_table.size() == size &&
		config.cy_max_table.size() == size;
}

bool finite_tables(const Core::Simulation::AerodynamicsConfig& config)
{
	return Common::all_finite(config.cx_zero_table) &&
		Common::all_finite(config.cy_alpha_table) &&
		Common::all_finite(config.roll_rate_max_table) &&
		Common::all_finite(config.alpha_max_table) &&
		Common::all_finite(config.cy_max_table);
}

bool positive_aerodynamic_limits(
	const Core::Simulation::AerodynamicsConfig& config)
{
	const bool positive_alpha = std::all_of(
		config.alpha_max_table.begin(),
		config.alpha_max_table.end(),
		[](double value) { return value > 0.0; });
	const bool positive_roll_rate = std::all_of(
		config.roll_rate_max_table.begin(),
		config.roll_rate_max_table.end(),
		[](double value) { return value > 0.0; });
	return positive_alpha && positive_roll_rate;
}
}

namespace Core
{
namespace Simulation
{
void validate_aerodynamics_config(const AerodynamicsConfig& config)
{
	const bool finite_scalars = Common::all_finite({
		config.wing_area, config.wingspan, config.length, config.height,
		config.mach_max, config.cy_zero, config.cz_beta, config.cx_gear,
		config.cx_airbrake, config.cx_flap, config.cx_lift_k,
		config.cx_alpha_k, config.cx_elevator_k, config.cy_flap,
		config.airbrake_pitch_comp_k
	});
	const bool valid_tables =
		Common::finite_strictly_increasing(config.mach_table) &&
		matching_table_sizes(config) && finite_tables(config) &&
		positive_aerodynamic_limits(config);
	if (!finite_scalars || !valid_geometry(config) || !valid_tables)
	{
		throw std::invalid_argument(
			"AerodynamicsConfig requires positive geometry and complete tables.");
	}
}

const AerodynamicsConfig& fck1c_aerodynamics_config()
{
	static const AerodynamicsConfig config = make_fck1c_config();
	return config;
}
}
}
