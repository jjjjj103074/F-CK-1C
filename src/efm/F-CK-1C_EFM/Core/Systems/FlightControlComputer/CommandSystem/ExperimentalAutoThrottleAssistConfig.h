#pragma once

// Developer-only feature configuration owned by the Command System boundary.

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
	double speed_kp_s_m = 0.0;
	double speed_ki_m_inv = 0.0;
	double speed_error_integral_limit_m = 0.0;
	double command_limit_normalized = 0.0;
	double mach_guard_throttle_rate_normalized_s = 0.0;
};
}
}
