#pragma once

#include "../../Database/InputOutputData.h"
#include "../../Database/ExternalActionQueue.h"
#include "../../Database/StoresData.h"
#include "../../StoresEvents.h"

namespace Core::Systems::Mc
{
/// @brief 管理全部固定掛點的觀測庫存、彈量與選站要求，並發布掛載變化。
class StoresManagement final
{
public:
	/// @brief 綁定 MC 持有的觀測、掛載分區、動作佇列與通知通道。
	StoresManagement(const MissionInputs& input,
	                 StoresData& stores,
	                 ExternalActionQueue& actions,
	                 StoresEvents& events);
	/// @brief 更新掛點庫存、機砲彈量及選站確認，再通知已完成的資料變化。
	void step(std::chrono::nanoseconds time, double dt_s);
	/// @brief 記錄目標掛點並遞增要求編號，供 Lua 執行及回報確認。
	/// @param station 目標掛點；空值表示清除選站要求。
	/// @param force 同一掛點也建立新要求。
	void request_station(std::optional<int> station, bool force = false);
	/// @brief 接收火控釋放通知，在既有彈量估算中扣除一枚空對空導彈。
	void record_missile_release();
	/// @brief 追加交由 Lua 執行的循環武器動作。
	void request_weapon_cycle();

private:
	/// @brief 更新來源批次與各掛點狀態，整理本次內容及有效性變化。
	InventoryChanged update_inventory(std::chrono::nanoseconds time);
	/// @brief 接收原生機砲彈量並更新來源有效性，回傳內容或有效性是否改變。
	bool update_gun_quantity(std::chrono::nanoseconds time);
	/// @brief 維持既有空對空總數來源優先序，並接收相符的選站確認。
	void update_legacy_summary();
	const MissionInputs& input_; ///< 原始掛載、API 彈量與選站確認。
	StoresData& stores_; ///< 本元件負責寫入的掛載分區。
	ExternalActionQueue& actions_;
	StoresEvents& events_;
};
}
