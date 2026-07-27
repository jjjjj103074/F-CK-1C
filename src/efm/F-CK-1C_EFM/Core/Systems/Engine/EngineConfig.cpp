#include "EngineConfig.h"

#include "../../../Common/ConfigValidation.h"

#include <stdexcept>

namespace
{
Core::Systems::EngineConfig make_fck1c_config()
{
	Core::Systems::EngineConfig config;
	config.start_time = 60.0;
	config.spool_up_tau = 2.5;
	config.spool_down_tau = 4.0;
	config.throttle_input_table = {
		0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0
	};
	config.power_table = {
		0.0, 0.01, 0.02, 0.06, 0.08, 0.1, 0.3, 0.5, 0.7, 0.9, 1.0
	};
	config.fuel_consumption_rate = 0.37;
	return config;
}

bool valid_engine_timing(const Core::Systems::EngineConfig& config)
{
	return config.start_time > 0.0 && config.spool_up_tau > 0.0 &&
		config.spool_down_tau > 0.0;
}

bool valid_engine_tables(const Core::Systems::EngineConfig& config)
{
	return Common::finite_strictly_increasing(
			config.throttle_input_table) &&
		config.power_table.size() == config.throttle_input_table.size() &&
		Common::all_finite(config.power_table);
}

bool valid_afterburner_range(
	const ::Systems::AfterburnerConfig& config)
{
	return config.detent > 0.0 && config.detent < 1.0 &&
		config.fuel_factor > 0.0 && config.core_rpm >= 0.0 &&
		config.core_rpm <= 1.0 &&
		config.light_throttle_output_min >= 0.0 &&
		config.light_throttle_output_min <= 1.0;
}

bool valid_afterburner_timing(
	const ::Systems::AfterburnerConfig& config)
{
	return config.core_drop_time > 0.0 && config.spool_in_tau > 0.0 &&
		config.spool_out_tau > 0.0;
}
}

namespace Core
{
namespace Systems
{
void validate_engine_config(const EngineConfig& config)
{
	const ::Systems::AfterburnerConfig& afterburner = config.afterburner;
	const bool finite_scalars = Common::all_finite({
		config.start_time, config.spool_up_tau, config.spool_down_tau,
		config.fuel_consumption_rate, afterburner.detent,
		afterburner.fuel_factor, afterburner.core_rpm,
		afterburner.core_drop_time, afterburner.spool_in_tau,
		afterburner.spool_out_tau,
		afterburner.light_throttle_output_min
	});
	const bool valid_afterburner =
		valid_afterburner_range(afterburner) &&
		valid_afterburner_timing(afterburner);
	if (!finite_scalars || !valid_engine_timing(config) ||
		!valid_engine_tables(config) || !valid_afterburner ||
		config.fuel_consumption_rate < 0.0)
	{
		throw std::invalid_argument(
			"EngineConfig requires valid timing, schedules, and fuel values.");
	}
}

const EngineConfig& fck1c_engine_config()
{
	static const EngineConfig config = make_fck1c_config();
	return config;
}
}
}
