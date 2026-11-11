#pragma once

#include "../../ModeEvents.h"
#include "Internal/TargetDesignation.h"
#include "Internal/WeaponOperation.h"
#include "../../Database/InputOutputData.h"
#include "../TimedAction.h"

namespace Core::Systems::Mc
{
/// @brief MC 火控入口，處理目標操作與武器使用程序。
class FireControl final
{
public:
	FireControl(const MissionInputs& input,
	            const ModeData& mode,
	            const WeaponUseData& execution,
	            FireControlData& data,
	            ExternalActionQueue& actions,
	            StoresCommandQueue& stores_commands);
	FireControl(const FireControl&) = delete;
	FireControl& operator=(const FireControl&) = delete;
	FireControl(FireControl&&) = delete;
	FireControl& operator=(FireControl&&) = delete;
	void handle(const Command& command);
	/// @brief 在 ModeData 更新後安排雷達模式與單次捕獲操作。
	void on_operating_mode_selected(const OperatingModeSelected& event);
	/// @brief 主保險選擇為 OFF/SIM 時停止武器程序。
	void on_master_arm_selected(const MasterArmSelected& event);
	void step(std::chrono::nanoseconds time, double dt_s);

private:
	enum class AcquisitionInput
	{
		Pulse,
		Hold
	};
	/// @brief 依操作情境將 TMS 與解籠輸入轉成目標操作。
	void handle_target_input(const Command& command);
	/// @brief 依解籠操作安排雷達與捕獲配合。
	void handle_uncage(bool pressed);
	/// @brief 將 TMS 上操作轉成脈衝或按住捕獲。
	void handle_tms_up(bool pressed);
	/// @brief 保存並送出捕獲操作方式。
	void apply_acquisition_mode(AirToAirSubmode mode);
	/// @brief 重送目前的空對空捕獲操作方式。
	void reapply_acquisition_mode();
	/// @brief 依雷達觀測安排開啟要求。
	void request_radar_on();
	/// @brief 送出捕獲要求，追蹤執行確認與保持時間。
	void acquire_target(AcquisitionInput input);
	/// @brief 送出切換目標要求，記錄指定意圖。
	void select_next_target();
	/// @brief 清除指定意圖，安排結束捕獲操作。
	void cancel_target_designation();
	/// @brief 送出捕獲放開要求，清除操作計時。
	void finish_acquisition_input();
	const MissionInputs& input_;
	const ModeData& mode_;
	FireControlData& data_;
	ExternalActionQueue& actions_;
	Internal::TargetDesignation target_designation_;
	Internal::WeaponOperation weapon_operation_;
	TimedAction acquisition_input_; ///< 捕獲操作的執行確認與保持計時。
	bool radar_on_requested_ = false;
};
}
