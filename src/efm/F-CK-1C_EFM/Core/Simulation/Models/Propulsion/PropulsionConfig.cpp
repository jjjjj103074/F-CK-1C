#include "PropulsionConfig.h"

#include "../../../../Common/ConfigValidation.h"
#include "../../../../Common/Table.h"

#include <stdexcept>

namespace
{
constexpr double kCarrierLaunchReferenceMach = 0.1;

Core::Simulation::PropulsionConfig make_fck1c_config()
{
	Core::Simulation::PropulsionConfig config;
	config.mach_table = {
		0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0
	};
	config.max_thrust_table_n = {
		54000.0, 53600.0, 53200.0, 52800.0, 52300.0, 51600.0,
		50800.0, 49900.0, 48900.0, 47800.0, 46600.0
	};
	config.afterburner_thrust_factor = 1.73;
	config.left_engine_position_body_m = { -3.793, -0.391, -0.716 };
	config.right_engine_position_body_m = { -3.793, -0.391, 0.716 };
	Core::Simulation::validate_propulsion_config(config);
	return config;
}
}

namespace Core
{
namespace Simulation
{
void validate_propulsion_config(const PropulsionConfig& config)
{
	const bool valid_scalars = Common::all_finite({
		config.afterburner_thrust_factor,
		config.left_engine_position_body_m.x,
		config.left_engine_position_body_m.y,
		config.left_engine_position_body_m.z,
		config.right_engine_position_body_m.x,
		config.right_engine_position_body_m.y,
		config.right_engine_position_body_m.z
	});
	if (!valid_scalars ||
		!Common::finite_strictly_increasing(config.mach_table) ||
		config.max_thrust_table_n.size() != config.mach_table.size() ||
		!Common::all_finite(config.max_thrust_table_n) ||
		config.afterburner_thrust_factor <= 0.0)
	{
		throw std::invalid_argument(
			"PropulsionConfig requires complete thrust tables and a positive "
			"afterburner factor.");
	}
}

const PropulsionConfig& fck1c_propulsion_config()
{
	static const PropulsionConfig config = make_fck1c_config();
	return config;
}

double fck1c_carrier_launch_reference_thrust_n()
{
	const PropulsionConfig& config = fck1c_propulsion_config();
	return Common::lerp(
		{
			config.mach_table.data(),
			config.max_thrust_table_n.data(),
			static_cast<unsigned>(config.mach_table.size())
		},
		kCarrierLaunchReferenceMach);
}
}
}
