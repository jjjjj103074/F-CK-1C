#pragma once

#include <vector>

namespace Systems
{
struct AfterburnerConfig
{
	double detent = 0.70;
	double fuel_factor = 2.2;
	double core_rpm = 0.94;
	double core_drop_time = 0.80;
	double spool_in_tau = 2.0;
	double spool_out_tau = 0.6;
	double light_throttle_output_min = 0.88;
};
}

namespace Core
{
namespace Systems
{
struct EngineConfig
{
	double start_time = 0.0;
	double spool_up_tau = 0.0;
	double spool_down_tau = 0.0;
	std::vector<double> throttle_input_table;
	std::vector<double> power_table;
	::Systems::AfterburnerConfig afterburner;
	double fuel_consumption_rate = 0.0;
};

void validate_engine_config(const EngineConfig& config);
const EngineConfig& fck1c_engine_config();
}
}

namespace Systems
{
using EngineConfig = Core::Systems::EngineConfig;
}
