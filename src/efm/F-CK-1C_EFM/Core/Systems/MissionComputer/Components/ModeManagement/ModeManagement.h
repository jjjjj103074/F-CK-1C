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
	/// DGFT/MSL 自動 ON、主保險 ON 回 NAV 等既有連動在此保留。
	void handle(const Command& command);

private:
	/// @brief 先更新軟體主保險，再以該階段的狀態副本通知所有訂閱者。
	void select_master_arm(MasterArmMode mode);
	/// @brief 更新模式並通知；重選相同模式也保留原有操作效果。
	/// @param cycle_weapon 是否保留此次 DGFT/MSL 選擇所要求的武器循環。
	void select_operating_mode(OperatingMode mode, bool cycle_weapon);
	ModeData& mode_; ///< MC 持有的共用狀態；本元件負責寫入。
	ModeEvents& events_; ///< MC 持有的同步通知通道。
};
}
