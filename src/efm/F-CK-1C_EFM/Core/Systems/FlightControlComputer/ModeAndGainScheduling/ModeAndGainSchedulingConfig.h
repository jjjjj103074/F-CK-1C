#pragma once

#include <array>
#include <vector>

namespace Systems
{
enum class StoresConfiguration
{
	Cat1,  // 輕載或空對空掛載的控制排程。
	Cat3  // 重載或空對地掛載的控制排程。
};

struct PilotInputShapingConfig
{
	double deadband_normalized = 0.0;  // 搖桿中心死區，正規化範圍 0 到 1。
	double command_time_constant_s = 0.0;  // 指令平滑的一階時間常數，單位秒。
	double command_rate_normalized_s = 0.0;  // 每秒允許的正規化指令變化率。
	double cubic_weight = 0.0;  // 三次曲線在輸入塑形中的混合比例。
};

struct ManeuverEnvelopeSchedule
{
	double maximum_roll_command_rad_s = 0.0;  // 飛行員可要求的最大滾轉角速度。
	double maximum_yaw_command_rad_s = 0.0;  // 飛行員可要求的最大偏航角速度。
	double soft_positive_load_factor_g = 0.0;  // 開始柔化正向過載指令的界線。
	double hard_positive_load_factor_g = 0.0;  // 不得超過的正向過載上限。
	double roll_rate_limit_rad_s = 0.0;  // 控制律允許的滾轉角速度上限。
	double pitch_rate_limit_rad_s = 0.0;  // 控制律允許的俯仰角速度上限。
	double yaw_rate_limit_rad_s = 0.0;  // 控制律允許的偏航角速度上限。
};

struct DirectionalControlSchedule
{
	double sideslip_damping_s_inv = 0.0;  // 側滑角轉為方向修正量的阻尼增益。
	double yaw_rate_damping = 0.0;  // 偏航角速度回授的阻尼增益。
};

struct StoresControlLawSchedule
{
	PilotInputShapingConfig pilot_input;  // 此掛載類別使用的駕駛輸入塑形。
	ManeuverEnvelopeSchedule envelope;  // 此掛載類別使用的機動包線。
	DirectionalControlSchedule directional;  // 此掛載類別使用的方向控制排程。
};

struct GainSchedulePoint
{
	double dynamic_pressure_pa = 0.0;  // 此排程點的動壓，單位 Pa。
	double command_gain = 0.0;  // 飛行員指令增益倍率。
	double damping_gain = 0.0;  // 角速度阻尼增益倍率。
	double limiter_gain = 0.0;  // 包線限制器增益倍率。
};

struct GainScheduleValues
{
	double command_gain = 1.0;  // 本週期插值後的指令增益倍率。
	double damping_gain = 1.0;  // 本週期插值後的阻尼增益倍率。
	double limiter_gain = 1.0;  // 本週期插值後的限制器增益倍率。
};

inline constexpr unsigned kGainScheduleSize = 4;

struct GLimiterOverrideConfig
{
	bool available = false;  // 是否允許開發用 G 限制解除指令生效。
	double margin_g = 2.0;  // 啟用解除後增加的正向 G 上限餘裕。
};

/// @brief CAT 選擇、增益排程與飛行包線的固定設定。
struct ModeAndGainSchedulingConfig
{
	StoresControlLawSchedule cat1;  // CAT I 的完整排程。
	StoresControlLawSchedule cat3;  // CAT III 的完整排程。
	std::array<GainSchedulePoint, kGainScheduleSize> gain_schedule;  // 動壓對三種增益的排程表。
	double stores_transition_time_constant_s = 0.0;  // CAT 排程切換的平滑時間常數。
	double guidance_bank_limit_rad = 0.0;  // 自動指引可要求的傾斜角上限。
	double guidance_roll_rate_limit_rad_s = 0.0;  // 自動指引傾斜參考值的變化率上限。
	double guidance_minimum_load_factor_g = 0.0;  // 自動指引可要求的最低法向過載。
	double guidance_maximum_load_factor_g = 0.0;  // 自動指引可要求的最高法向過載。
	double hard_bank_limit_rad = 0.0;  // 所有來源共同遵守的傾斜角硬限制。
	double hard_minimum_load_factor_g = 0.0;  // 所有來源共同遵守的最低法向過載。
	std::vector<double> angle_of_attack_mach;  // 迎角限制表的馬赫數軸。
	std::vector<double> angle_of_attack_limit_rad;  // 各馬赫數對應的巡航迎角硬限制。
	double cruise_angle_of_attack_blend_start_rad = 0.0;  // 巡航迎角保護開始介入的位置。
	double landing_angle_of_attack_blend_start_rad = 0.0;  // 起落架放下時迎角保護開始介入的位置。
	double landing_angle_of_attack_limit_rad = 0.0;  // 起落架放下時的迎角硬限制。
	GLimiterOverrideConfig g_limiter_override;  // 本模組擁有的開發用包線覆蓋設定。
};

/// @brief 建立 F-CK-1C 正式使用的 CAT、增益與飛行包線設定。
/// @return 含完整正式調校值的模式與增益設定。
ModeAndGainSchedulingConfig make_fck1c_mode_and_gain_scheduling_config();

/// @brief 驗證模式、增益排程與飛行包線模組自己的固定參數。
/// @param config 要檢查的模式與增益設定。
/// @throws std::invalid_argument 排程、包線或開發覆蓋設定無效時擲出。
void validate_mode_and_gain_scheduling_config(
	const ModeAndGainSchedulingConfig& config);
}
