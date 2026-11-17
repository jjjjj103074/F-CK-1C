#pragma once

#include "../../ModeEvents.h"
#include "../../../../Contracts/Commands.h"

namespace Core::Systems::Mc
{
/// @brief MC 模式資料的唯一寫入者；處理主保險與模式選擇，再同步發布具型別事件。
/// MC 組裝訂閱者並設定通知順序。
class ModeManagement final
{
public:
	/// @brief 綁定 MC 持有的模式資料與事件通道。
	/// @param mode 本元件唯一寫入、其他使用者以 const 參照讀取的模式分區。
	/// @param events 本台 MC 的同步通道；兩個參照目標的生命週期須涵蓋本元件。
	ModeManagement(ModeData& mode, ModeEvents& events);
	/// @brief 處理模式與主保險的按下指令。
	/// 返回前完成此次選擇的全部同步通知，供外層繼續處理下一筆 FIFO 指令。
	void handle(const Command& command);

private:
	/// @brief 先更新軟體主保險，再以該階段的狀態副本通知所有訂閱者。
	void select_master_arm(MasterArmMode mode);
	/// @brief 在超控中央位置處理主模式選擇；重按 A-A／A-G 回到 NAV。
	void select_master_mode(MasterMode mode);
	/// @brief 更新超控位置並發布生效模式；中央位置恢復保存的主模式。
	void select_override(OverrideMode mode);
	/// @brief 以更新前後的完整狀態同步通知模式訂閱者。
	void publish_selection(const ModeData& previous);
	ModeData& mode_; ///< MC 持有的共用狀態；本元件負責寫入。
	ModeEvents& events_; ///< MC 持有的同步通知通道。
};
}
