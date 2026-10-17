#pragma once

#include <vector>

namespace Systems
{
struct AfterburnerConfig
{
	double detent_normalized = 0.70;
	double fuel_factor = 2.2;
	double core_rpm_0_1 = 0.94;
	double core_drop_time_s = 0.80;
	double spool_in_tau_s = 2.0;
	double spool_out_tau_s = 0.6;
	double light_throttle_output_min_normalized = 0.88;
};
}

namespace Core
{
namespace Systems
{
struct EngineConfig
{
	double start_time_s = 0.0;
	double spool_up_tau_s = 0.0;
	double spool_down_tau_s = 0.0;
	std::vector<double> throttle_input_table_normalized;
	std::vector<double> power_table_normalized;
	::Systems::AfterburnerConfig afterburner;
	double fuel_consumption_rate_kg_s = 0.0;
};

void validate_engine_config(const EngineConfig& config);
const EngineConfig& fck1c_engine_config();
}
}

namespace Systems
{
using EngineConfig = Core::Systems::EngineConfig;
}
