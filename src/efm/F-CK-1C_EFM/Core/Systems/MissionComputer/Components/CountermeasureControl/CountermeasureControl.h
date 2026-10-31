#pragma once

#include "../../Database/ExternalActionQueue.h"
#include "../../ModeEvents.h"
#include "../../../../Contracts/Commands.h"

namespace Core::Systems::Mc
{
/// @brief 管理反制啟動資格與中止要求；實際施放程序由現有 Lua 反制系統執行。
/// 由 CMS 指令與主保險選擇事件驅動。
class CountermeasureControl final
{
public:
	/// @brief 綁定唯讀模式與可追加的外部動作佇列；兩者由 MC 持有且須持續存在。
	CountermeasureControl(const ModeData& mode, ExternalActionQueue& actions);
	/// @brief ON 時接受前／後／左啟動要求；右向操作發出中止要求。
	void handle(const Command& command);
	/// @brief 依本次主保險事件的狀態，在 OFF/SIM 時追加 CMS 中止要求。
	/// MC 保證先通知火控停止，再通知本元件，接著才處理 NAV 選擇。
	void on_master_arm_selected(const MasterArmSelected& event);

private:
	const ModeData& mode_; ///< 啟動資格判斷使用的唯讀模式。
	ExternalActionQueue& actions_; ///< 追加交由 Bridge／Lua 執行的反制語意動作。
};
}
