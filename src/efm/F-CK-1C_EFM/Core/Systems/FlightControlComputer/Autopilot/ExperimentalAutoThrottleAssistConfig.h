#pragma once

namespace Core
{
namespace Systems
{
struct ExperimentalAutoThrottleAssistConfig
{
	double maximum_mach = 0.0;
	double disconnect_mach = 0.0;
	double speed_step_mps = 0.0;
	double minimum_target_speed_mps = 0.0;
	double maximum_target_speed_mps = 0.0;
	double base_command_normalized = 0.0;
	double speed_kp = 0.0;
	double speed_ki = 0.0;
	double speed_error_integral_limit_m = 0.0;
	double command_limit_normalized = 0.0;
	double mach_guard_throttle_rate_per_s = 0.0;
};
}
}
