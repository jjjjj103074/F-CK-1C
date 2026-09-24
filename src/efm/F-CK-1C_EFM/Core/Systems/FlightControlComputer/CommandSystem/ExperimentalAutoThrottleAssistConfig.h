#pragma once

// Developer-only feature configuration owned by the Command System boundary.

namespace Core::Systems
{
/// @brief 實驗性自動油門輔助的速度控制與馬赫數保護參數。
struct ExperimentalAutoThrottleAssistConfig
{
	double maximum_mach = 0.0;  // 開始降低油門的馬赫數門檻。
	double disconnect_mach = 0.0;  // 強制解除輔助的馬赫數門檻。
	double speed_step_mps = 0.0;  // 每次目標速度調整量。
	double minimum_target_speed_mps = 0.0;  // 可選擇的最低目標速度。
	double maximum_target_speed_mps = 0.0;  // 可選擇的最高目標速度。
	double base_command_normalized = 0.0;  // 無速度誤差時的基準油門命令。
	double speed_kp_s_m = 0.0;  // 速度誤差比例增益。
	double speed_ki_m_inv = 0.0;  // 速度誤差積分增益。
	double speed_error_integral_limit_m = 0.0;  // 速度誤差積分狀態的限制。
	double command_limit_normalized = 0.0;  // 自動油門命令的正規化上限。
	double mach_guard_throttle_rate_normalized_s = 0.0;  // 馬赫數保護降低油門的最大速率。
};
}
