#pragma once

#include "Database/MissionData.h"

#include <sigslot/signal.hpp>

namespace Core::Systems::Mc
{
/// @brief 軟體主保險的選擇事件；重選相同值仍會通知。
/// 保存本次選擇前後的狀態副本。
struct MasterArmSelected
{
	ModeData previous; ///< 此次選擇前的狀態。
	ModeData selected; ///< 已寫入資料庫、與本次通知對應的狀態。
};

/// @brief NAV／DGFT／MSL 的選擇事件，包含重選時仍須執行的操作意圖。
struct OperatingModeSelected
{
	ModeData previous; ///< 此次選擇前的狀態。
	ModeData selected; ///< 已寫入資料庫、與本次通知對應的狀態。
	bool cycle_weapon = false; ///< 本次選擇要求執行武器循環。
};

/// @brief 單台 MC 持有的同步事件通道；訂閱關係與連線生命週期集中在 MC 組裝處。
/// 同一執行緒依序完成每筆指令及其通知。
/// 訂閱者依 group 編號由小到大執行。
/// 回呼接收事件參照，需保存時複製事件內容。
struct ModeEvents final
{
	ModeEvents() = default;
	ModeEvents(const ModeEvents&) = delete;
	ModeEvents& operator=(const ModeEvents&) = delete;
	ModeEvents(ModeEvents&&) = delete;
	ModeEvents& operator=(ModeEvents&&) = delete;

	sigslot::signal_st<const MasterArmSelected&> master_arm_selected;
	sigslot::signal_st<const OperatingModeSelected&> operating_mode_selected;
};
}
