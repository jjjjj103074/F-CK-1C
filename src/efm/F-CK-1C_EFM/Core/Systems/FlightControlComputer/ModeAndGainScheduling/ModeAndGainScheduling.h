#pragma once

#include "ModeAndGainSchedulingConfig.h"
#include "../CommandSystem/FlightControlCommandBinding.h"

#include <vector>

// Public boundary for stores configuration, gain scheduling and envelopes.

namespace Systems
{
struct FlightGuidanceEnvelope
{
	double bank_limit_rad = 0.0;  // 自動飛行指引可要求的最大傾斜角。
	double roll_rate_limit_rad_s = 0.0;  // 自動飛行指引可要求的最大滾轉角速度。
	double minimum_normal_acceleration_g = 0.0;  // 自動飛行指引可要求的最低法向過載。
	double maximum_normal_acceleration_g = 0.0;  // 自動飛行指引可要求的最高法向過載。
};

struct HardProtectionEnvelope
{
	double bank_limit_rad = 0.0;  // 所有指令來源共同遵守的傾斜角上限。
	double minimum_normal_acceleration_g = 0.0;  // 所有指令來源共同遵守的最低法向過載。
	double maximum_normal_acceleration_g = 0.0;  // 所有指令來源共同遵守的最高法向過載。
	double angle_of_attack_blend_start_rad = 0.0;  // 迎角保護開始混合的位置。
	double angle_of_attack_limit_rad = 0.0;  // 迎角硬限制。
	double roll_rate_limit_rad_s = 0.0;  // 滾轉角速度硬限制。
	double pitch_rate_limit_rad_s = 0.0;  // 俯仰角速度硬限制。
	double yaw_rate_limit_rad_s = 0.0;  // 偏航角速度硬限制。
};

struct ManeuverEnvelope
{
	FlightGuidanceEnvelope guidance;  // 自動飛行指引使用的較窄工作範圍。
	HardProtectionEnvelope hard_protection;  // 所有控制來源共同遵守的硬限制。
};

struct ActiveFlightControlConfiguration
{
	StoresConfiguration stores_configuration = StoresConfiguration::Cat1;  // 目前選定的掛載類別。
	double stores_transition_0_1 = 0.0;  // CAT I 至 CAT III 的平滑混合比例。
	StoresControlLawSchedule stores;  // 本週期插值後的掛載控制排程。
	GainScheduleValues gains;  // 本週期按動壓選出的增益。
	ManeuverEnvelope envelope;  // 本週期可用的指引與硬性飛行包線。
};

struct ModeAndGainSchedulingStepInput
{
	double dt_s = 0.0;  // 本次主週期時間，單位秒。
	double dynamic_pressure_pa = 0.0;  // 本次動壓，單位 Pa。
	double mach = 0.0;  // 本次馬赫數。
	bool update_slow_gain_schedule = false;  // 本週期是否重新計算慢頻增益。
	bool landing_gear_handle_down = false;  // 起落架把手是否位於放下位置。
};

struct ManeuverEnvelopeInput
{
	const ModeAndGainSchedulingConfig& config;  // 完整模式與增益設定。
	const StoresControlLawSchedule& stores;  // 本週期使用的掛載控制排程。
	double mach = 0.0;  // 本次馬赫數。
	bool developer_g_limiter_override_active = false;  // 開發用 G 限制解除是否生效。
	double developer_g_limiter_override_margin_g = 0.0;  // 啟用解除後增加的 G 餘裕。
	bool landing_gear_handle_down = false;  // 起落架把手是否位於放下位置。
};

class ModeAndGainScheduling
{
public:
	/// @brief 建立 CAT、增益排程與飛行包線模組。
	/// @param config 已完成驗證的模組設定；建構後由本物件保存副本。
	explicit ModeAndGainScheduling(
		const ModeAndGainSchedulingConfig& config);

	/// @brief 提供 CAT 選擇與開發用 G 限制解除指令綁定。
	/// @return 本模組擁有的指令識別碼及交付函式。
	std::vector<Core::Systems::FlightControlCommandBinding> command_bindings();

	/// @brief 依本週期飛行條件更新 CAT 混合、增益與飛行包線。
	/// @param input 本週期時間、動壓、馬赫數與模式狀態。
	/// @return 本物件持有的有效設定；下次更新會覆寫內容。
	const ActiveFlightControlConfiguration& update(
		const ModeAndGainSchedulingStepInput& input);

	/// @brief 指定目標掛載控制類別。
	/// @param configuration 後續週期要平滑切換到的 CAT 類別。
	void set_stores_configuration(StoresConfiguration configuration);

	/// @brief 在 CAT I 與 CAT III 間切換目標類別。
	/// @param command_pressed 指令是否已超過按下門檻。
	void toggle_stores_configuration(bool command_pressed);

	/// @brief 讀取目前目標掛載控制類別。
	/// @return CAT I 或 CAT III。
	StoresConfiguration target_stores_configuration() const;

	/// @brief 讀取目前 CAT I 至 CAT III 的平滑混合比例。
	/// @return 0 代表 CAT I，1 代表 CAT III，中間值代表過渡中。
	double transition_0_1() const;

	/// @brief 讀取開發用 G 限制解除目前是否生效。
	/// @return 設定允許且已接收啟用指令時為 true。
	bool g_limiter_override_active() const;

private:
	const ModeAndGainSchedulingConfig config_;  // 本架飛機的固定模式與增益設定。
	StoresConfiguration target_ = StoresConfiguration::Cat1;  // 指令選定的目標 CAT 類別。
	ActiveFlightControlConfiguration active_;  // 最近一次完整更新結果。
	bool g_limiter_override_active_ = false;  // 本架飛機目前的開發用包線覆蓋狀態。
};

ManeuverEnvelope make_maneuver_envelope(
	const ManeuverEnvelopeInput& input);
void validate_maneuver_envelope(const ManeuverEnvelope& envelope);
}
